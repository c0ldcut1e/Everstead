package website

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"slices"
	"sync"
	"time"
)

const releasesURL = "https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/"

var platformNames = map[string]string{"windows-x64": "Windows x86-64", "linux-x64": "Linux x86-64", "linux-arm64": "Linux ARM64"}

var platformOrder = []string{"windows-x64", "linux-x64", "linux-arm64"}

type versionDownload struct {
	Platform string
	URL      string
	Size     string
}

type versionEntry struct {
	Version string
	Latest  bool
	Clients []versionDownload
	Servers []versionDownload
}

type releaseFile struct {
	File string `json:"file"`
	Size int64  `json:"size"`
}

type versionManifest struct {
	Releases map[string]releaseFile `json:"releases"`
	Servers  map[string]releaseFile `json:"servers"`
}

type versionCache struct {
	mutex    sync.Mutex
	entries  []versionEntry
	loadedAt time.Time
}

var versionHTTP = &http.Client{Timeout: 10 * time.Second}

func fetchJSON(url string, target any) error {
	response, err := versionHTTP.Get(url)
	if err != nil {
		return err
	}

	defer response.Body.Close()
	if response.StatusCode != http.StatusOK {
		return fmt.Errorf("%s returned %d", url, response.StatusCode)
	}

	return json.NewDecoder(response.Body).Decode(target)
}

func formatSize(size int64) string {
	return fmt.Sprintf("%.1f MB", float64(size)/(1024*1024))
}

func downloadsFor(version string, files map[string]releaseFile) []versionDownload {
	var downloads []versionDownload
	for _, platform := range platformOrder {
		file, ok := files[platform]
		if !ok {
			continue
		}

		downloads = append(downloads, versionDownload{
			Platform: platformNames[platform],
			URL:      releasesURL + "versions/" + version + "/" + file.File,
			Size:     formatSize(file.Size),
		})
	}

	return downloads
}

func loadVersions() ([]versionEntry, error) {
	var root struct {
		Latest   string   `json:"latest"`
		Versions []string `json:"versions"`
	}
	if err := fetchJSON(releasesURL+"manifest.json", &root); err != nil {
		return nil, err
	}

	entries := make([]versionEntry, len(root.Versions))
	var group sync.WaitGroup
	for i, version := range root.Versions {
		group.Add(1)
		go func() {
			defer group.Done()
			entries[i] = versionEntry{Version: version, Latest: version == root.Latest}
			var manifest versionManifest
			if err := fetchJSON(releasesURL+"versions/"+version+"/manifest.json", &manifest); err != nil {
				log.Printf("website: load version %s: %v", version, err)
				return
			}

			entries[i].Clients = downloadsFor(version, manifest.Releases)
			entries[i].Servers = downloadsFor(version, manifest.Servers)
		}()
	}

	group.Wait()
	slices.Reverse(entries)
	return entries, nil
}

// We cache the list because building it costs one R2 request per version
func (cache *versionCache) get() []versionEntry {
	cache.mutex.Lock()
	defer cache.mutex.Unlock()
	if cache.entries != nil && time.Since(cache.loadedAt) < 10*time.Minute {
		return cache.entries
	}

	entries, err := loadVersions()
	if err != nil {
		log.Printf("website: load versions: %v", err)
		return cache.entries
	}

	cache.entries = entries
	cache.loadedAt = time.Now()
	return entries
}

func (w *Website) versions(writer http.ResponseWriter, request *http.Request) {
	w.render(writer, "versions.html", pageData{Title: "Version archive", Account: w.signedIn(request), Versions: w.versionCache.get()})
}
