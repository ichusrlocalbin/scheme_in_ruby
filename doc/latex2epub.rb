#!/usr/bin/env ruby
# -*- coding: utf-8 -*-
def latex2epub
  latex_out_file = 'epub.tex'
  epub_file = 'book.epub'
  latex_files = ['book.tex',
                 'introduction.tex',
                 'eval.tex',
                 'function.tex',
                 'recursion.tex',
                 'extend_language.tex',
                 'next_step.tex',
                 'references.bib',
                 'bib.tex']
  out_file = open(latex_out_file, 'w')
  latex_files.each do |latex_file|
    open(latex_file) do |file|
      file.each do |line|
        out_file.puts line.
          gsub(/\\hspace\{-3mm\}/, '').
          gsub(/\.eps/, '.png').
          gsub(/\[htbp\]/, '').
          gsub(/\\begin\{breakitembox\}\[l\]/, '\begin{breakitembox}').
          gsub(/\\begin\{document\}/, $warning).
          gsub(/\{0.4\\textwidth\}/, '')
        #         gsub(/\\begin\{breakitembox\}\[l\]/, '<div class="column">').
        #         gsub(/\\end\{breakitembox\}/, '</div>')
      end
    end
  end
  out_file.close()
  `pandoc -f latex -t epub #{latex_out_file} -o #{epub_file}`
end
$warning =<<EOS
\\begin{document}
\\begin{center}{\\Huge 注意}
\\end{center}
このepubフォーマットは試作版です。リンクやコードの表示などに不具合があります。
正式版であるpdf版を次のURLから取得してください。
\\verb|https://github.com/ichusrlocalbin/scheme_in_ruby|
EOS

latex2epub
