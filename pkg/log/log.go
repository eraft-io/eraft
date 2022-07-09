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

package log

import (
	"fmt"
	"io"
	"strings"

	log "github.com/phuslu/log"
)

var MainLogger *log.Logger

func init() {
	MainLogger = &log.Logger{
		Level: log.ParseLevel("debug"),
		// Writer: &log.FileWriter{
		// 	Filename: "./logs/main.log",
		// 	MaxSize:  500 * 1024 * 1024,
		// 	Cleaner: func(filename string, maxBackups int, matches []os.FileInfo) {
		// 		var dir = filepath.Dir(filename)
		// 		var total int64
		// 		for i := len(matches) - 1; i >= 0; i-- {
		// 			total += matches[i].Size()
		// 			if total > 5*1024*1024*1024 {
		// 				os.Remove(filepath.Join(dir, matches[i].Name()))
		// 			}
		// 		}
		// 	},
		// },
		Writer: &log.ConsoleWriter{
			Formatter: func(w io.Writer, a *log.FormatterArgs) (int, error) {
				return fmt.Fprintf(w, "%c%s %s %s] %s\n%s", strings.ToUpper(a.Level)[0],
					a.Time, a.Goid, a.Caller, a.Message, a.Stack)
			},
		},
	}
}
