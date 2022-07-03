package clientsdk

type ClientSdk struct {
}

func (c *ClientSdk) UploadFile(localPath string) error {
	// 1.split file to blocks

	// 2.call ServerGroupMeta query with servers

	// 3.call WriteFileBlock write to block server

	// 4.call FileBlockMeta write file meta data to metaserver
	return nil
}

func (c *ClientSdk) DownloadFile(path string) ([]byte, error) {
	//1.FileBlockMeta find file block meta

	//2.call ServerGroupMeta query with servers

	//3.call ReadFileBlock to read all file blocks
	return nil, nil
}
