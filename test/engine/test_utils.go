package engine

import (
	"io/ioutil"
	"os"
	"path"
)

func RemoveDir(dir string) {
	dirs, err := ioutil.ReadDir(dir)
	if err != nil {
		panic(err.Error())
	}
	for _, d := range dirs {
		os.RemoveAll(path.Join([]string{dir, d.Name()}...))
	}
}
