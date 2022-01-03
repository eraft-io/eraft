package persister
 
import (
   "fmt"
   "github.com/syndtr/goleveldb/leveldb"
   "log"
)


type Persister struct{
	path string
	db *leveldb.DB	
}


//打开数据库
func( p *Persister ) Init(path string)  {
	var err error
	//数据存储路径和一些初始文件
	p.db,err = leveldb.OpenFile(path,nil)
	if err != nil {
		log.Fatalln(err)
	}
}


func( p *Persister )Put(key string,value string)  {
		p.db.Put([]byte(key),[]byte(value),nil)
		//p.db.Put(key ,[]byte(value),nil)
}
		
func( p *Persister ) Get(key string) []byte  {		
	value,err := p.db.Get([]byte(key),nil)
	if err != nil {
		log.Println(err)
		return nil
	}
	return value
}
		
func ( p *Persister )PrintStrVal(key string)  {
	value := p.Get(key)
	fmt.Println(string(value))
}
	

