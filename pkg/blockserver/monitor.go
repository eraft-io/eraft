// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package blockserver

import (
	"time"

	"github.com/eraft-io/eraft/pkg/log"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
	"github.com/shirou/gopsutil/mem"
)

func RecordMetrics() {
	go func() {
		for {
			opsProcessed.Inc()
			time.Sleep(1 * time.Second)
			v, err := mem.VirtualMemory()
			if err != nil {
				log.MainLogger.Error().Msgf("get virtual memory stat error %s", err.Error())
			}
			totalMemory.Set(float64(v.Total))
			memoryAvailable.Set(float64(v.Available))
			memoryUsedPercent.Set(float64(v.UsedPercent))
		}
	}()
}

var (
	opsProcessed = promauto.NewCounter(prometheus.CounterOpts{
		Name: "wellwood_test_processed_ops_total",
		Help: "The total number of processed events",
	})
	totalMemory = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "wellwood_host_total_memory",
		Help: "host total memory",
	})
	memoryAvailable = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "wellwood_host_available_memory",
		Help: "host available memory",
	})
	memoryUsedPercent = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "wellwood_host_memory_used_percent",
		Help: "host memory used percent",
	})
)
