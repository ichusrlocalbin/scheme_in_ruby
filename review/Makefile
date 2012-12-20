all: pdf

pdf:
	rm -rf book-pdf book.pdf
	review-pdfmaker config.yml

epub:
	rm -rf book-epub book.epub
	review-epubmaker config.yml

clean:
	rm -rf book-pdf book.pdf book-epub book.epub
