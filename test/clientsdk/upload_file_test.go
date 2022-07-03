package clientsdk

import (
	"testing"

	client "github.com/eraft-io/eraft/pkg/clientsdk"
)

func TestUpload(t *testing.T) {
	clientSdk := client.ClientSdk{}
	clientSdk.UploadFile("/Users/colin/Desktop/full_course_reader.pdf")
}
