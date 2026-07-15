// Command ppm2img converts a binary (P6) PPM image to a JPEG file.
//
// It exists because the ENIAC panel photos in images/*.ppm are plain P6 PPM,
// a format the Go standard library cannot decode, but the eniacweb-server
// frontend needs a web-friendly format. Usage:
//
//	go run ./tools/ppm2img src.ppm dst.jpg [src2.ppm dst2.jpg ...]
package main

import (
	"bufio"
	"fmt"
	"image"
	"image/color"
	"image/jpeg"
	"os"
)

func readToken(r *bufio.Reader) (string, error) {
	// Skip whitespace and '#' comments between header tokens.
	for {
		b, err := r.ReadByte()
		if err != nil {
			return "", err
		}
		if b == '#' {
			for {
				b, err = r.ReadByte()
				if err != nil {
					return "", err
				}
				if b == '\n' {
					break
				}
			}
			continue
		}
		if b == ' ' || b == '\t' || b == '\n' || b == '\r' {
			continue
		}
		tok := []byte{b}
		for {
			b, err = r.ReadByte()
			if err != nil {
				return "", err
			}
			if b == ' ' || b == '\t' || b == '\n' || b == '\r' {
				break
			}
			tok = append(tok, b)
		}
		return string(tok), nil
	}
}

func decodePPM(path string) (image.Image, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	r := bufio.NewReader(f)
	magic, err := readToken(r)
	if err != nil {
		return nil, fmt.Errorf("%s: reading magic: %w", path, err)
	}
	if magic != "P6" {
		return nil, fmt.Errorf("%s: unsupported PPM magic %q (only binary P6 is supported)", path, magic)
	}
	width, err := readIntToken(r)
	if err != nil {
		return nil, fmt.Errorf("%s: reading width: %w", path, err)
	}
	height, err := readIntToken(r)
	if err != nil {
		return nil, fmt.Errorf("%s: reading height: %w", path, err)
	}
	maxval, err := readIntToken(r)
	if err != nil {
		return nil, fmt.Errorf("%s: reading maxval: %w", path, err)
	}
	if maxval != 255 {
		return nil, fmt.Errorf("%s: unsupported maxval %d (only 255 is supported)", path, maxval)
	}

	img := image.NewRGBA(image.Rect(0, 0, width, height))
	row := make([]byte, width*3)
	for y := 0; y < height; y++ {
		if _, err := readFull(r, row); err != nil {
			return nil, fmt.Errorf("%s: reading pixel data at row %d: %w", path, y, err)
		}
		for x := 0; x < width; x++ {
			img.Set(x, y, color.RGBA{R: row[x*3], G: row[x*3+1], B: row[x*3+2], A: 255})
		}
	}
	return img, nil
}

func readIntToken(r *bufio.Reader) (int, error) {
	tok, err := readToken(r)
	if err != nil {
		return 0, err
	}
	var n int
	if _, err := fmt.Sscanf(tok, "%d", &n); err != nil {
		return 0, fmt.Errorf("parsing %q as int: %w", tok, err)
	}
	return n, nil
}

func readFull(r *bufio.Reader, buf []byte) (int, error) {
	n := 0
	for n < len(buf) {
		m, err := r.Read(buf[n:])
		n += m
		if err != nil {
			return n, err
		}
	}
	return n, nil
}

func convert(src, dst string) error {
	img, err := decodePPM(src)
	if err != nil {
		return err
	}
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer out.Close()
	if err := jpeg.Encode(out, img, &jpeg.Options{Quality: 85}); err != nil {
		return err
	}
	return out.Close()
}

func main() {
	args := os.Args[1:]
	if len(args) == 0 || len(args)%2 != 0 {
		fmt.Fprintln(os.Stderr, "usage: ppm2img src.ppm dst.jpg [src2.ppm dst2.jpg ...]")
		os.Exit(2)
	}
	status := 0
	for i := 0; i < len(args); i += 2 {
		src, dst := args[i], args[i+1]
		if err := convert(src, dst); err != nil {
			fmt.Fprintf(os.Stderr, "convert %s -> %s: %v\n", src, dst, err)
			status = 1
			continue
		}
		fmt.Printf("%s -> %s\n", src, dst)
	}
	os.Exit(status)
}
