package website

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"image"
	"image/png"
	"io"
	"net/http"
	"slices"

	"github.com/everstead/backend/internal/errcode"
	"github.com/everstead/backend/internal/store"
	"go.mongodb.org/mongo-driver/bson"
)

const maxTextureBytes = 64 << 10

// We re-encode uploads so only plain decoded pixels get stored and served back
func readTexture(request *http.Request, field string, sizes []image.Point) (string, []byte, bool) {
	file, _, err := request.FormFile(field)
	if err != nil {
		return "", nil, false
	}

	defer file.Close()

	decoded, err := png.Decode(io.LimitReader(file, maxTextureBytes))
	if err != nil {
		return "", nil, false
	}

	valid := false
	for _, size := range sizes {
		if decoded.Bounds().Size() == size {
			valid = true
		}
	}

	if !valid {
		return "", nil, false
	}

	var buffer bytes.Buffer
	if err := png.Encode(&buffer, decoded); err != nil {
		return "", nil, false
	}

	sum := sha256.Sum256(buffer.Bytes())
	return hex.EncodeToString(sum[:]), buffer.Bytes(), true
}

func (w *Website) changeSkin(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	request.Body = http.MaxBytesReader(writer, request.Body, maxTextureBytes*2)
	if err := request.ParseMultipartForm(maxTextureBytes * 2); err != nil {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	model := request.PostFormValue("model")
	if model != store.SkinModelClassic && model != store.SkinModelSlim {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	fields := bson.M{"skinModel": model}
	if len(request.MultipartForm.File["skin"]) > 0 {
		hash, data, ok := readTexture(request, "skin", []image.Point{{64, 64}, {64, 32}})
		if !ok {
			w.failAccount(writer, request, account, errcode.SkinInvalid)
			return
		}

		if err := w.store.SaveTexture(request.Context(), hash, data); err != nil {
			w.failAccount(writer, request, account, errcode.SkinUpdate)
			return
		}

		fields["skinHash"] = hash
	}

	if request.PostFormValue("reset") != "" {
		fields["skinHash"] = ""
	}

	if err := w.store.SetAccountSkin(request.Context(), account.Pid, fields); err != nil {
		w.failAccount(writer, request, account, errcode.SkinUpdate)
		return
	}

	http.Redirect(writer, request, "/account?notice=skin", http.StatusSeeOther)
}

func (w *Website) changeCape(writer http.ResponseWriter, request *http.Request) {
	account := w.signedIn(request)
	if account == nil {
		http.Redirect(writer, request, "/login", http.StatusSeeOther)
		return
	}

	if err := request.ParseForm(); err != nil {
		w.failAccount(writer, request, account, errcode.BadRequestBody)
		return
	}

	cape := request.PostFormValue("cape")
	if cape != "" && !slices.Contains(account.Capes, cape) {
		w.failAccount(writer, request, account, errcode.CapeLocked)
		return
	}

	if err := w.store.SetAccountSkin(request.Context(), account.Pid, bson.M{"cape": cape}); err != nil {
		w.failAccount(writer, request, account, errcode.SkinUpdate)
		return
	}

	http.Redirect(writer, request, "/account?notice=cape", http.StatusSeeOther)
}

func (w *Website) texture(writer http.ResponseWriter, request *http.Request) {
	data, err := w.store.Texture(request.Context(), request.PathValue("hash"))
	if err != nil {
		http.NotFound(writer, request)
		return
	}

	writer.Header().Set("Content-Type", "image/png")
	writer.Header().Set("Cache-Control", "public, max-age=31536000, immutable")
	writer.Write(data)
}
