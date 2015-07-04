= 少し言語を拡張して

前章までで本質的なことはすべて学んだと言いました。ただし、このままでは実際のプログラムが書きづらいのも確かです。この章では、我々の言語を書きやすくするため、いくつかの機能を追加していきます。重要な機能は前章で学び終わっていますので、気軽に読んでみて下さい。

== リスト

ここではリストを扱えるようにします@<fn>{fn1}。ここで紹介する関数は、本来のSchemeではリストに限らず使えるものですが、今回はリストを対象に機能を限定します。

リストは空リストもしくはリストに先頭の要素を加えたものとして構造的帰納法で定義されます。Rubyでは配列で表現します。

@<tt>{null?}は与えたリストが空リストか調べるものです。空リストは@<tt>{:nil}で表されます。その他、後で説明するリスト用の組み込み関数も環境として定義しておきます。

//emlist{
def null?(list)
  list == []
end

$list_env = {
  :nil   => [],
  :null? => [:prim, lambda{|list| null?(list)}],
  :cons  => [:prim, lambda{|a, b| cons(a, b)}],
  :car   => [:prim, lambda{|list| car(list)}],
  :cdr   => [:prim, lambda{|list| cdr(list)}],
  :list  => [:prim, lambda{|*list| list(*list)}],
}

$global_env = [$list_env, $primitive_fun_env, $boolean_env]
//}

@<tt>{cons}は、リストに先頭要素を加えます。リスト以外のものに要素を加えようとすると、我々の不十分な処理系はエラーを返します。

//emlist{
def cons(a, b)
  if not list?(b)
    raise "sorry, we haven't implemented yet..."
  else
    [a] + b
  end
end
//}

(以前定義していますが)@<tt>{car}、@<tt>{cdr}はそれぞれリストの先頭の要素、および先頭の要素を除いたリストを返します。

//emlist{
def car(list)
  list[0]
end

def cdr(list)
  list[1..-1]
end
//}

@<tt>{list}は、与えられたリストをそのまま返します。Rubyで可変長引数は配列で渡されますので、配列をリストとして用いているため、そのままの値を使うことができるためです。

//emlist{
def list(*list)
  list
end
//}


== 定義

ここでは定義を扱います。次の式を考えてみましょう。

//emlist{
[:define, :id, [:lambda, [:x], :x]]
//}

これは、@<tt>{:id}を引き数で与えられたものをそのまま返す関数として定義するものです。したがって、その後、下の式を評価すると、

//emlist{
[:id, 3]
//}

3が返されることを期待します。

もう一つ、異なる記述方法の定義を導入します。このプログラムは上の定義と同じ意味を持ちます。

//emlist{
[:define, [:id, :x], :x]
//}

みなさんには、こちらの方がなじみがあるかと思います。関数名に続き仮引数と、それに続く関数のボディから成ります。

これを実装するためにはどうすれば良いでしょうか。変数に定義する値を束縛した環境を付け加えます。ポイントは、その後の評価でもその定義が使えるように環境を書き換える必要があることです。また、すでに変数が束縛されている場合には値を書き換えるようにします。これらは環境を代入により上書きします(関数名が!を使う関数を呼んでいる点に注意しましょう)。

//emlist{
def eval_define(exp, env)
  if define_with_parameter?(exp)
    var, val = define_with_parameter_var_val(exp)
  else
    var, val = define_var_val(exp)
  end
  var_ref = lookup_var_ref(var, env)
  if var_ref != nil
    var_ref[var] = _eval(val, env)
  else
    extend_env!([var], [_eval(val, env)], env)
  end
  nil
end

def extend_env!(parameters, args, env)
  alist = parameters.zip(args)
  h = Hash.new
  alist.each { |k, v| h[k] = v }
  env.unshift(h)
end

def define_with_parameter?(exp)
  list?(exp[1])
end

def define_with_parameter_var_val(exp)
  var = car(exp[1])
  parameters, body = cdr(exp[1]), exp[2]
  val = [:lambda, parameters, body]
  [var, val]
end

def define_var_val(exp)
  [exp[1], exp[2]]
end

def lookup_var_ref(var, env)
  env.find{|alist| alist.key?(var)}
end  

def define?(exp)
  exp[0] == :define
end
//}

準備ができましたので下のようなリストを扱うプログラムをいろいろと実行してみましょう@<fn>{fn2}。

//emlist{
[:define, [:length, :list], 
 [:if, [:null?, :list], 
  0, 
  [:+, [:length, [:cdr, :list]], 1]]]
[:length, [:list, 1, 2]]
//}


== cond式

条件分岐はif式で記述できますが、条件が多くなると、ifのネストが深くなり、プログラムが見づらいものになっていきます。そこで次のような式を実行できるcondを導入します。

//emlist{
[:cond, 
 [[:>, 1, 1], 1],
 [[:>, 2, 1], 2],
 [[:>, 3, 1], 3],
 [:else, -1]]
//}

この式は上から、リストの左の条件式を順に評価し、真になればその右の式を評価値をcond式の値とします。偽であれば、その下のリストに対して同様のことを行います。@<tt>{:else}があった場合は、その右の式を値とします。リストはいくつあっても構いません。この場合は2が返り値となります。

この実装はif式に書き換え、それを評価するだけです。

//emlist{
def eval_cond(exp, env)
  if_exp = cond_to_if(cdr(exp))
  eval_if(if_exp, env)
end

def cond_to_if(cond_exp)
  if cond_exp == []
    ''
  else
    e = car(cond_exp)
    p, c = e[0], e[1]
    if p == :else
      p = :true
    end
    [:if, p, c, cond_to_if(cdr(cond_exp))]
  end  
end

def cond?(exp)
  exp[0] == :cond
end
//}


== パーサー

ここまでプログラムを書いてきて、プログラムが書きづらかったことでしょう。プログラムをRubyで評価しやすいように、Rubyの配列を用いてパーサーを省略するとともに、Rubyのシンボルを用いてシンボルテーブルを省略していたためです。LispやSchemeはよくカッコのお化けと言われますが、今まさにその表記法に移る時が来ました。

次のように「@<tt>{()}」を使う本来のSchemeの記述方法プログラムをRubyの文字列として入力すると、今までと同じように「@<tt>{[]}」や「@<tt>{,}」を使ったRubyのデータ型に変換するものを作ります。変換後のデータを評価させれば今までどおりの結果が得られますので、ユーザは「@<tt>{()}」を使う普通のSchemeのプログラムを入力できるようになります。

//emlist{
_eval(parse('(define (length list) (if  (null? list) 0 (+ (length (cdr list)) 1)))'), 
      $global_env)
puts _eval(parse('(length (list 1 2 3))'), $global_env)
//}

これは、「@<tt>{(}」「@<tt>{)}」を「@<tt>{[}」「@<tt>{]}」に、変数をRubyのシンボルに置き換えるため「@<tt>{:}」を変数の先頭に追加し、空白を「@<tt>{,}」に置換するより実現します。

//emlist{
def parse(exp)
  program = exp.strip().
    gsub(/[a-zA-Z\+\-\*><=][0-9a-zA-Z\+\-=!*]*/, ':\\0').
    gsub(/\s+/, ', ').
    gsub(/\(/, '[').
    gsub(/\)/, ']')
  eval(program)
end
//}


== quote

次に追加する機能は@<tt>{quote}です。次のようにリストを引数として与えるときなどで便利です。

//emlist{
puts _eval(parse('(length (quote (1 2 3)))'), $global_env)
//}

@<tt>{quote}の引数は評価せずに引数をそのまま評価値として返します@<fn>{fn3}。

その他の例を挙げてみます。プログラムを引数とする関数を書きたい場合、通常であれば引数が評価されてその関数に渡されます。しかし、その評価の方法をその関数で書きたいので、評価値ではなく式そのものを関数に渡したいのです。このような場合に@<tt>{quote}は役立ちます。

//emlist{
def eval_quote(exp, env)
  car(cdr(exp))
end

def quote?(exp)
  exp[0] == :quote
end
//}

それでは今まで、拡張してきた機能が動作するようにしましょう。

//emlist{
def special_form?(exp)
  lambda?(exp) or 
    let?(exp) or 
    letrec?(exp) or 
    if?(exp) or 
    cond?(exp) or 
    define?(exp) or 
    quote?(exp)
end

def eval_special_form(exp, env)
  if lambda?(exp)
    eval_lambda(exp, env)
  elsif let?(exp)
    eval_let(exp, env)
  elsif letrec?(exp)
    eval_letrec(exp, env)
  elsif if?(exp)
    eval_if(exp, env)
  elsif cond?(exp)
    eval_cond(exp, env)
  elsif define?(exp)
    eval_define(exp, env)
  elsif quote?(exp)
    eval_quote(exp, env)
  end
end
//}


== REPL

最後にインタープリタと呼ばれるにふさわしい処理を付け加えます。インタープリタはユーザと対話しながらプログラムを作成することができる点に特徴があります。この機能、すなわち、ユーザから入力を読み取り(Read)、その結果を評価し(Eval)、その結果を表示する(Print)ことを繰り返す(Loop)機能です。これは頭文字をとって、REPLとも呼ばれます。

実現は上の機能をそのまま単純に実装します。ここで、@<tt>{pp}@<fn>{fn4}という式を整形する処理を新たに定義しています。

//emlist{
def repl
  prompt = '>>> '
  second_prompt = '> ' 
  while true
    print prompt
    line = gets or return
    while line.count('(') > line.count(')') 
      print second_prompt
      next_line = gets or return
      line += next_line
    end
    redo if line =~ /\A\s*\z/m 
    begin
      val = _eval(parse(line), $global_env)
    rescue Exception => e
      puts e.to_s
      redo
    end
    puts pp(val)
  end
end

def closure?(exp)
  exp[0] == :closure
end

def pp(exp)
  if exp.is_a?(Symbol) or num?(exp)
    exp.to_s
  elsif exp == nil
    'nil'
  elsif exp.is_a?(Array) and closure?(exp)
    parameter, body, env = exp[1], exp[2], exp[3]
    "(closure #{pp(parameter)} #{pp(body)})"
  elsif exp.is_a?(Array) and lambda?(exp)
    parameters, body = exp[1], exp[2]
    "(lambda #{pp(parameters)} #{pp(body)})"
  elsif exp.is_a?(Hash)
    if exp == $primitive_fun_env
      '*prinmitive_fun_env*'
    elsif exp == $boolean_env
      '*boolean_env*'
    elsif exp == $list_env
      '*list_env*'
    else
      '{' + exp.map{|k, v| pp(k) + ':' + pp(v)}.join(', ') + '}'
    end
  elsif exp.is_a?(Array)
    '(' + exp.map{|e| pp(e)}.join(', ') + ')'
  else 
    exp.to_s
  end
end
//}

それでは実行してみましょう。

//emlist{
>> repl
>>> (define (fib n) (if (< n 2) n  (+ (fib (- n 1)) (fib (- n 2)))))
nil
>>> (fib 10)     
55
//}

実行できました。

今までよりはずいぶん楽になるのではないでしょうか。


== その他

他に不便なところはありませんか。まだまだあるでしょう。実現していない機能は多々あります。@<tt>{named let}や@<tt>{let*}などの機能を調べその実装にトライしてみて下さい。友達に速度が遅いと言われたら、コンパイラを作りましょう。Haskellのように遅延評価でないとと言われたら、遅延評価にしてしまいましょう。他の言語のこの機能がない、と言われたら、自分で追加してしまいましょう。自分でプログラミング言語を作ったからこそ味わえるおもしろさです。大いに使い倒して下さい。


== まとめ

この章では、プログラミングを便利にするような次の機能を実現しました。

 * defineによる定義
 * リスト
 * cond式
 * パーサー
 * quote
 * REPL


//footnote[fn1][今回我々が作成しようとしているSchemeの元言語であるLispという名はList Processingすなわちリスト処理から由来しています。それほど、リストを扱うのが得意な言語なのです。]

//footnote[fn2][実行する前に4.5節で示す@<tt>{special_form?}と@<tt>{eval_special_form}を追記する必要があります。]

//footnote[fn3][@<tt>{quote}は通常、@<tt>{'}を使って簡易に記載できます。すなわち、@<tt>{(quote 1 2 3)}は@<tt>{'(1 2 3)}と同じものです。これは処理系が@<tt>{'}を読み込んだ時に、@<tt>{quote}に展開することで実現されています。腕に自信のある方はぜひこの機能の実装にチャレンジしてみて下さい。Ruby 1.9から正規表現でカッコの対応付けが可能になっています。]

//footnote[fn4][@<tt>{pretty print}の略です。]
