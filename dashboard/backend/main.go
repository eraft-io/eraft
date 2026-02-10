package main

import (
	"encoding/json"
	"flag"
	"log"
	"net/http"
	"strings"
	"time"
)

var (
	port           = flag.String("port", "8080", "Dashboard HTTP port")
	configAddrs    = flag.String("config-addrs", "", "Comma-separated config cluster addresses")
	updateInterval = flag.Duration("update-interval", 5*time.Second, "Status update interval")
)

type Server struct {
	collector *Collector
	cache     *DashboardResponse
	cacheLock chan struct{}
}

func NewServer(collector *Collector) *Server {
	return &Server{
		collector: collector,
		cacheLock: make(chan struct{}, 1),
	}
}

func (s *Server) StartBackgroundCollector() {
	ticker := time.NewTicker(*updateInterval)
	go func() {
		for {
			select {
			case <-ticker.C:
				data, err := s.collector.CollectDashboardData()
				if err != nil {
					log.Printf("Failed to collect dashboard data: %v", err)
					continue
				}

				select {
				case s.cacheLock <- struct{}{}:
					s.cache = data
					<-s.cacheLock
				default:
					// Skip update if lock is held
				}
			}
		}
	}()
}

func (s *Server) HandleDashboard(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	select {
	case s.cacheLock <- struct{}{}:
		defer func() { <-s.cacheLock }()

		if s.cache == nil {
			data, err := s.collector.CollectDashboardData()
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			s.cache = data
		}

		json.NewEncoder(w).Encode(s.cache)
	default:
		// Return cached data if update is in progress
		if s.cache != nil {
			json.NewEncoder(w).Encode(s.cache)
		} else {
			http.Error(w, "Dashboard data not ready", http.StatusServiceUnavailable)
		}
	}
}

func (s *Server) HandleTopology(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	select {
	case s.cacheLock <- struct{}{}:
		defer func() { <-s.cacheLock }()
		if s.cache != nil {
			json.NewEncoder(w).Encode(s.cache.Topology)
		} else {
			http.Error(w, "Data not ready", http.StatusServiceUnavailable)
		}
	default:
		if s.cache != nil {
			json.NewEncoder(w).Encode(s.cache.Topology)
		} else {
			http.Error(w, "Data not ready", http.StatusServiceUnavailable)
		}
	}
}

func (s *Server) HandleMetrics(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")

	select {
	case s.cacheLock <- struct{}{}:
		defer func() { <-s.cacheLock }()
		if s.cache != nil {
			json.NewEncoder(w).Encode(s.cache.Metrics)
		} else {
			http.Error(w, "Data not ready", http.StatusServiceUnavailable)
		}
	default:
		if s.cache != nil {
			json.NewEncoder(w).Encode(s.cache.Metrics)
		} else {
			http.Error(w, "Data not ready", http.StatusServiceUnavailable)
		}
	}
}

func main() {
	flag.Parse()

	if *configAddrs == "" {
		log.Fatal("config-addrs is required")
	}

	configAddrList := strings.Split(*configAddrs, ",")
	collector := NewCollector(configAddrList)

	server := NewServer(collector)
	server.StartBackgroundCollector()

	// Serve static files
	fs := http.FileServer(http.Dir("dashboard/frontend/build"))
	http.Handle("/", fs)

	// API endpoints
	http.HandleFunc("/api/dashboard", server.HandleDashboard)
	http.HandleFunc("/api/topology", server.HandleTopology)
	http.HandleFunc("/api/metrics", server.HandleMetrics)

	log.Printf("Dashboard server starting on port %s", *port)
	log.Printf("Config cluster: %v", configAddrList)

	if err := http.ListenAndServe(":"+*port, nil); err != nil {
		log.Fatal(err)
	}
}
