= μSchemeR上でSchemeを動かす

== @<i>{p}scheme in μschemeR

ここでは、 @<hd>{next_step|Scheme in μSchemeRにチャレンジ} で述べたように、μSchemeR上でSchemeの一部分を作ってみます。便器上、このSchemeを@<i>{p}schemeと名づけます(pはpicoから取りました)。繰り返しになりますが、この目的は関数言語を使ったプログラミング方法を習得することです。ぜひ、実際に手を動かしてプログラムを動作させて下さい。

== μschemeRの拡張

まずは、μschemeRを拡張します。必要な機能はμschemeRの記述に使ったRubyのコードに対応する機能です。と、言ってもさほど多くはなく、組み込み関数をいくつか定義する程度です。

boolean演算に@<tt>{not}を追加します。

//emlist{
$boolean_env = {
  :true => true, :false => false,
  :not  => [:prim, lambda{|e| not e }]
}
//}

リスト演算にリスト判定@<tt>{list?}とconsペアでないことを判定する@<tt>{atom?}を追加します。μSchemeRではconsペアでリストしか作れませんが、実際のSchemeはそうではありません。そこで、リストかどうかを判定するのではなく、明にconsペアでないことをソースコード上で示すために@<tt>{atom?}を導入します。

//emlist{
$list_env = {
  :nil      => [],
  :null?    => [:prim, lambda{|list|  null?(list)}],
  :cons     => [:prim, lambda{|a, b|  cons(a, b)}],
  :car      => [:prim, lambda{|list|  car(list)}],
  :cdr      => [:prim, lambda{|list|  cdr(list)}],
  :list     => [:prim, lambda{|*list| list(*list)}],
  :list?    => [:prim, lambda{|exp|   list?(exp)}],
  :atom?    => [:prim, lambda{|exp|   atom?(exp)}]
}
//}

//emlist{
def atom?(exp)
  not exp.is_a?(Array)
end
//}

@<tt>{_eval}関数も@<tt>{atom?}を使うよう書き換えます。

//emlist{
def _eval(exp, env)
  if atom?(exp)
    if immediate_val?(exp)
      exp
    else 
      lookup_var(exp, env)
    end
  else
    if special_form?(exp)
      eval_special_form(exp, env)
    else
      fun = _eval(car(exp), env)
      args = eval_list(cdr(exp), env)
      apply(fun, args)
    end
  end
end
//}

組み込み関数に比較演算@<tt>{eq?}, 数字判定@<tt>{number?}, 関数適用@<tt>{_apply_primitive_fun}を追加します。

//emlist{
$primitive_fun_env = {
  :+       => [:prim, lambda{|x, y| x + y}],
  :-       => [:prim, lambda{|x, y| x - y}],
  :*       => [:prim, lambda{|x, y| x * y}],
  :>       => [:prim, lambda{|x, y| x > y}],
  :>=      => [:prim, lambda{|x, y| x >= y}],
  :<       => [:prim, lambda{|x, y| x <  y}],
  :<=      => [:prim, lambda{|x, y| x <= y}],
  :==      => [:prim, lambda{|x, y| x == y}],
  :eq?     => [:prim, lambda{|x, y| x == y}],
  :number? => [:prim, lambda{|exp| num?(exp)}],
  :_apply_primitive_fun \
       => [:prim, lambda{|fun, args| _apply_primitive_fun(fun, args)}],
}
//}

関数適用@<tt>{_apply_primitive_fun}では、Rubyの関数@<tt>{_apply_primitive_fun(fun, args)}を呼び出しています。ここでは、与えられたシンボルに対応する組み込み関数を呼び出しています。参考として、今まで利用していた@<tt>{apply_primitive_fun}も記載します。少しややこしいですが、@<tt>{_apply_primitive_fun(fun, args)}をはじめとする関数を定義し、これらを環境の中で@<tt>{_apply_primitive_fun}などに束縛することにより、μSchemeR上でこの名前で呼び出すことが可能となります。これから作成する@<i>{p}schemeを動作させるときにも使用することができるようになります。

//emlist{
def _apply_primitive_fun(fun_symbol, args)
  apply_primitive_fun($primitive_fun_env[fun_symbol], args)
end

def apply_primitive_fun(fun, args)
  fun_val = fun[1]
  fun_val.call(*args)
end
//}

最後に、クオート文字「@<tt>{'}」を含む式 @<tt>{'(exp)}を @<tt>{(quote exp)}に展開する機能を作成します。@<tt>{macro_quote}でRuby 1.9から導入された再帰型の正規表現を使っています@<fn>{fn2}。これにより、ネストされたクオート文字も正しく展開できるようになります。パーサも@<tt>{macro_quote}を呼ぶよう修正します。この他、関数名にアンダースコア「@<tt>{_}」も使えるようにパーサーを修正しています。

//footnote[fn2][以降、Rubyはバージョン1.9以上を使って下さい。]

//emlist{
def macro_quote(s)
  s = s.clone
  while s.sub!(/'([^()]+|(?<pa>\((\?:\s|[^()]+|\g<pa>)*\)))/) {|m|
      m.sub(/^'(.*)$/, "(quote \\1)")
    }
  end
  s
end

def parse(exp)
  program = macro_quote(exp.strip()).
    gsub(/[a-zA-Z_\+\-\*><=][0-9a-zA-Z_\+\-=!*]*/, ':\\0').
    gsub(/\s+/, ', ').
    gsub(/\(/, '[').
    gsub(/\)/, ']')
  eval(program)
end
//}

== @<i>{p}schemeの実装

それでは、μSchemeRで@<i>{p}schemeを実装していきます。

μSchemeRを動作させるプログラムを読み込ませた後、@<tt>{repl}でプロンプトを表示し、μSchemeRが動作する環境でプログラムを動かして下さい。

//emlist{
repl
>>> 
//}

最初は、リストを扱う式の拡張です。これらは@<tt>{car}と@<tt>{cdr}の組み合わせから成ります。名前の付け方は、最初と最後をそれぞれcとrを除き、引数のリストに適応する順に@<tt>{car}であれば @<tt>{a}を、@<tt>{cdr}であれば @<tt>{d}を右から追加しています。例えば、@<tt>{cdar}は@<tt>{car}したリストを@<tt>{cdr}して返す関数です。

//emlist{
(define (caar list)
  (car (car list)))

(define (cdar list)
  (cdr (car list)))

(define (cadr list)
  (car (cdr list)))

(define (cadar list)
  (car (cdr (car list))))

(define (caddr list)
  (car (cdr (cdr list))))

(define (cadddr list)
  (car (cdr (cdr (cdr list)))))
//}

次はリストを扱う関数として、与えられた2つのリストを結合する@<tt>{append}と、与えられたリストを逆順にする@<tt>{reverse}を定義します。

//emlist{
(define (append list1 list2)
  (if (null? list1)
      list2
      (cons (car list1) (append (cdr list1) list2))))
//}

//emlist{
(define (reverse l)
  (if (null? l)
      '()
      (append (reverse (cdr l)) (list (car l)))))
//}

@<hd>{recursion|関数型言語と再帰 – for文はどこへ?} でも述べましたが、リストは空リストもしくはリストに先頭の要素を加えたものとして構造的帰納法で定義されます。この構造どおりに関数を記述することにより、全てのリストに対して関数を定義することができます。

徐々に高度になっていきます。次は、関数を扱う関数、すなわち高階関数を扱います。

@<tt>{map} は、与えられたリストの要素を与えられた関数へ適応し、その結果をリストとして返す関数です。

//emlist{
(define (map fun list)
  (if (null? list) 
      '()
      (cons (fun (car list)) (map fun (cdr list)))))
//}

@<tt>{zip}は二つのリストの対応する要素同士をペアとしてそれらのリストを返す関数です。

//emlist{
(define (zip list1 list2)
  (if (null? list1)
      '()
      (cons (list (car list1) (car list2)) (zip (cdr list1) (cdr list2)))))
//}

ここまでの関数を見て、ほとんど同じような形をしているのに気づきましたか?それを示すために、関数@<tt>{fold}を定義します。@<tt>{fold}は、与えられたリストが空であれば、与えられた演算結果@<tt>{accum}を返します。そうでなければ、@<tt>{accum}とリストの先頭要素を与えられた関数に適応し、それを演算結果として、cdrしたリストとともに再帰的に@<tt>{fold}関数を呼び出します。

//emlist{
(define (fold fun accum list)
  (if (null? list)
      accum
      (fold fun (fun accum (car list)) (cdr list))))
//}

この関数を使って、上の関数を書き直すことができます。例えば、@<tt>{map}は次のように書き直すことが出来ます。

//emlist{
(define (map_fold fun list)
  (let ((_fun (lambda (accum list_element) (cons (fun list_element) accum))))
    (reverse (fold _fun '() list))))
//}

ここで、@<tt>{_fun}を与えられたリスト要素をmap関数の引数として渡された関数@<tt>{fun}に適用し、その結果と演算結果@<tt>{accum}のconsペアを返す関数に束縛しています。この関数と、初期演算結果として空リスト、与えられたリストとを引数にして@<tt>{fold}関数を呼び出しています。注意すべきは、最後に@<tt>{reverse}関数で結果を並び替えている点です。リストの先頭要素から関数@<tt>{fun}が適応され、consペアが作成されていくことから、引数のリスト@<tt>{list}と@<tt>{fold}の結果のリストとの順番が逆になるためです。

頭の中で動きを理解できたら、ぜひ、@<tt>{append}, @<tt>{reverse}, @<tt>{zip}を@<tt>{fold}を使って書き換えてみて下さい。高階関数を使う良い演習になると思います。

このように、高階関数を扱うことが出来るプログラミング言語では、関数の中に同じ構造を見つけたら、それを抽象化した関数を作成し書き換えることが出来ます。抽象化していく中で、何が本質的な計算なのか見えてくることもありますので、どんどん試してみてください。

準備の最後に、ハッシュを扱う関数群を定義します。これらは、空のハッシュを作り出す@<tt>{make_hash}, キーと値のペアを設定する@<tt>{hash_put}, 与えたキーの値を取得する@<tt>{hash_get}から成ります。

//emlist{
(define (make_hash)
  (quote ()))

(define (hash_put hash key val)
  (cons (list key val) hash))

(define (hash_get hash key)
  (cond ((null? hash) nil)
	((eq? (caar hash) key) (cadar hash))
	(else (hash_get (cdr hash) key))))
//}

例えば、次のように使うことが出来ます。

//emlist{
>>> (let ((hash (hash_put (hash_put (make_hash) (quote key1) (quote value1)) 
		      (quote key2) (quote value2))))
      (hash_get hash (quote key1)))
> > value1
//}

大変お待たせしました。@<i>{p}schemeを実装しましょう。と言っても、実はほとんどやることはありません。μSchemeRのコードを見ながら移植していけば良いだけです。一気にコードを書いてしまいます。

//emlist{
(define primitive_fun_frame
  (quote ((+  +) (-  -) (*  *)
	  (nil  nil) 
	  (car  car) (cdr  cdr) (cons  cons) 
	  (eq?  eq?)
	  (list list) (null? null?) (list? list?)
	  (num? number?)
	  (true true) (false false)
	  )))

(define global_env 
  (list primitive_fun_frame))

(define (atom? exp)
  (not (list? exp)))
	
(define (__eval exp env)
  (let ((dummy (print (list (quote __eval) exp))))
  (if (atom? exp)
      (if (immediate_val? exp)
	  exp
	  (lookup_var exp env))
      (if (special_form? exp)
	  (eval_special_form exp env)
	  (let ((fun (__eval (car exp) env))
		(args (eval_list (cdr exp) env)))
	      (_apply fun args))))))

(define (special_form? exp)
  (lambda? exp))

(define (lambda? exp)
  (eq? (quote lambda) (car exp)))

(define (eval_special_form exp env)
  (if (lambda? exp)
      (eval_lambda exp env)
      nil))

(define (eval_lambda exp env)
  (make_closure exp env))

(define (make_closure exp env)
  (let ((parameters (cadr  exp))
	(body       (caddr exp)))
    (list (quote closure) parameters body env)))

(define (primitive_fun? fun)
  (not (list? fun)))

(define (_apply fun args)
  (let ((dummy (print (list (quote _apply) fun args))))
  (if (primitive_fun? fun)
      (apply_primitive_fun fun args)
      (lambda_apply fun args))))

(define (apply_primitive_fun fun args)
  (_apply_primitive_fun fun args))

(define (lambda_apply closure args)
  (let ((parameters (cadr   closure))
	(body       (caddr  closure))
	(env        (cadddr closure)))
    (let ((new_env (extend_env parameters args env)))
      (__eval body new_env))))

(define (eval_list exp_list env)
  (let ((eval_env (lambda (exp) (__eval exp env))))
    (map eval_env exp_list)))

(define (immediate_val? exp)
  (number? exp))

(define (extend_env parameters args env)
  (let ((hash (make_hash)))
    (let ((extend_hash (lambda (hash parameter_arg)
		       (hash_put hash (car parameter_arg) (cadr parameter_arg)))))
      (let ((frame (reverse (fold extend_hash hash (zip parameters args)))))
	(cons frame env)))))

(define (lookup_var_frame var frame)
  (hash_get frame var))

(define (lookup_var var env)
  (if (null? env)
      nil
      (let ((val (lookup_var_frame var (car env))))
	(if (null? val)
	    (lookup_var var (cdr env))
	    val))))
//}

いかがでしょうか。末尾に参考としてμSchemeRのコードをつけますので、それと見比べて見てください。ほとんど同じコードであることが分かると思います(μSchemeRのコードはこれを見越して書いたので当然なのですが)。言い換えれば、この章を読む前にすでにμSchemeRを実装するRubyのコードで関数型言語のプログラミング方法について学んでいたことになります。

それでは完成した処理系を動かして見ましょう。

//emlist{
>>> (__eval '((lambda (x) (- ((lambda (x) x) 2) x)) 1) global_env)
1
//}

確かに正しい結果が返って来ました。Rubyの処理系の上に、μSchemeRの処理系を作成し、その上に@<i>{p}schemeの処理系を作成し、その上で@<i>{p}schemeプログラムを動かしました。おめでとうございます。

どの処理系で何が動いているかを確かめるために、@<tt>{(__eval '(num? 1) global_env)}というプログラムを例に考えて見て下さい。特に、@<tt>{num?}, @<tt>{number?}や引数 1がどのように評価されて行くか、@<tt>{apply_primitive_fun}, @<tt>{_apply_primitive_fun}がどこで関数適用されるのか、結果はどのように返るのかを追ってみて下さい。

まだ、@<tt>{letrec}など未実装の機能もありますが、それは読者の演習としたいと思います。

== まとめ

この章では次のことを学びました。

 * μSchemeRの処理系を用いたSchemeのサブセット @<i>{p}schemeの処理系の実装方法
 * @<i>{p}schemeの処理系はμSchemeRとほぼ同様に実装できること
 * μSchemeR言語でのプログラミングによる関数型言語のプログラミング方法

== 参考: μSchemeRのプログラム

//emlist{
$DEBUG2 = true
$primitive_fun_env = {
  :+       => [:prim, lambda{|x, y| x + y}],
  :-       => [:prim, lambda{|x, y| x - y}],
  :*       => [:prim, lambda{|x, y| x * y}],
  :>       => [:prim, lambda{|x, y| x > y}],
  :>=      => [:prim, lambda{|x, y| x >= y}],
  :<       => [:prim, lambda{|x, y| x <  y}],
  :<=      => [:prim, lambda{|x, y| x <= y}],
  :==      => [:prim, lambda{|x, y| x == y}],
  :eq?     => [:prim, lambda{|x, y| x == y}],
  :number? => [:prim, lambda{|exp| num?(exp)}],
  :_apply_primitive_fun \
       => [:prim, lambda{|fun, args| _apply_primitive_fun(fun, args)}],
}

$boolean_env = {
  :true => true, :false => false,
  :not  => [:prim, lambda{|e| not e }]
}

$list_env = {
  :nil      => [],
  :null?    => [:prim, lambda{|list|  null?(list)}],
  :cons     => [:prim, lambda{|a, b|  cons(a, b)}],
  :car      => [:prim, lambda{|list|  car(list)}],
  :cdr      => [:prim, lambda{|list|  cdr(list)}],
  :list     => [:prim, lambda{|*list| list(*list)}],
  :list?    => [:prim, lambda{|exp|   list?(exp)}],
  :atom?    => [:prim, lambda{|exp|   atom?(exp)}]
}

$debug_env = {
  :print => [:prim, lambda{|exp| puts(exp.inspect) if $DEBUG2; exp}]
}

$global_env = [$list_env, $primitive_fun_env, $boolean_env, $debug_env]

def null?(list)
  list == []
end

def atom?(exp)
  not exp.is_a?(Array)
end

def cons(a, b)
  if not list?(b)
    raise "sorry, we haven't implemented yet..."
  else
    [a] + b
  end
end

def car(list)
  list[0]
end

def cdr(list)
  list[1..-1]
end

def list?(exp)
  exp.is_a?(Array)
end

def list(*list)
  list
end

def macro_quote(s)
  s = s.clone
  while s.sub!(/'([^()]+|(?<pa>\((\?:\s|[^()]+|\g<pa>)*\)))/) {|m|
      m.sub(/^'(.*)$/, "(quote \\1)")
    }
  end
  s
end

def parse(exp)
  program = macro_quote(exp.strip()).
    gsub(/[a-zA-Z_\+\-\*><=][0-9a-zA-Z_\+\-=!*]*/, ':\\0').
    gsub(/\s+/, ', ').
    gsub(/\(/, '[').
    gsub(/\)/, ']')
  eval(program)
end

def apply(fun, args)
  log "apply fun:#{pp(fun)}, args:#{pp(args)}"
  if primitive_fun?(fun)
    apply_primitive_fun(fun, args)
  else
    lambda_apply(fun, args)
  end
end

def immediate_val?(exp)
  num?(exp) 
end

def num?(exp)
  exp.is_a?(Numeric)
end

def primitive_fun?(exp)
  exp[0] == :prim
end

def lambda?(exp)
  exp[0] == :lambda
end

def make_closure(exp, env)
  parameters, body = exp[1], exp[2]
  [:closure, parameters, body, env]
end

def closure_to_parameters_body_env(closure)
  [closure[1], closure[2], closure[3]]
end

def lambda_apply(closure, args)
  parameters, body, env = closure_to_parameters_body_env(closure)
  new_env = extend_env(parameters, args, env)
  _eval(body, new_env)
end

def extend_env(parameters, args, env)
  alist = parameters.zip(args)
  h = Hash.new
  alist.each { |k, v| h[k] = v }
  [h] + env
end

def extend_env!(parameters, args, env)
  alist = parameters.zip(args)
  h = Hash.new
  alist.each { |k, v| h[k] = v }
  env.insert(0, h)
end

def lookup_var(var, env)
  alist = env.find{|alist| alist.key?(var)}
  if alist == nil
    raise "couldn't find value to variables:'#{var}'"
  end
  alist[var]
end  

def lookup_var_ref(var, env)
  env.find{|alist| alist.key?(var)}
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

def define_var_val(exp)
  [exp[1], exp[2]]
end

def _apply_primitive_fun(fun_symbol, args)
  apply_primitive_fun($primitive_fun_env[fun_symbol], args)
end

def apply_primitive_fun(fun, args)
  fun_val = fun[1]
  fun_val.call(*args)
end

def special_form?(exp)
  lambda?(exp) or 
    let?(exp) or 
    letrec?(exp) or 
    if?(exp) or 
    cond?(exp) or 
    define?(exp) or 
    quote?(exp) or 
    setq?(exp)
end

def quote?(exp)
  exp[0] == :quote
end

def define?(exp)
  exp[0] == :define
end

def cond?(exp)
  exp[0] == :cond
end

def let?(exp)
  exp[0] == :let
end

def letrec?(exp)
  exp[0] == :letrec
end

def if?(exp)
  exp[0] == :if
end

def eval_lambda(exp, env)
  make_closure(exp, env)
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
  elsif setq?(exp)
    eval_set!(exp, env)
  end
end

def eval_set!(exp, env)
  var, val = setq_to_var_val(exp)
  var_ref = lookup_var_ref(var, env)
  if var_ref != nil
    var_ref[var] = _eval(val, env)
  else
    raise "undefined variable:'#{var}'"    
  end
  nil
end

def setq_to_var_val(exp)
  [exp[1], exp[2]]
end

def setq?(exp)
  exp[0] == :set!
end

def eval_quote(exp, env)
  car(cdr(exp))
end

def if_to_cond_true_false(exp)
  [exp[1], exp[2], exp[3]]
end

def eval_if(exp, env)
  cond, true_clause, false_clause = if_to_cond_true_false(exp)
  if _eval(cond, env)
    _eval(true_clause, env)
  else
    _eval(false_clause, env)
  end
end

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

def eval_let(exp, env)
  parameters, args, body = let_to_parameters_args_body(exp)
  new_exp = [[:lambda, parameters, body]] + args
  _eval(new_exp, env)
end

def eval_letrec(exp, env)
  parameters, args, body = letrec_to_parameters_args_body(exp)
  tmp_env = Hash.new
  parameters.each do |parameter| 
    tmp_env[parameter] = :dummy
  end
  ext_env = extend_env(tmp_env.keys(), tmp_env.values(), env)
  args_val = eval_list(args, ext_env)
  set_extend_env!(parameters, args_val, ext_env)
  new_exp = [[:lambda, parameters, body]] + args
  _eval(new_exp, ext_env)
end

def set_extend_env!(parameters, args_val, ext_env)
  parameters.zip(args_val).each do |parameter, arg_val|
    ext_env[0][parameter] = arg_val
  end
end

def let_to_parameters_args_body(exp)
  [exp[1].map{|e| e[0]}, exp[1].map{|e| e[1]}, exp[2]]
end

def letrec_to_parameters_args_body(exp)
  let_to_parameters_args_body(exp)
end

def _eval(exp, env)
  if atom?(exp)
    if immediate_val?(exp)
      exp
    else 
      lookup_var(exp, env)
    end
  else
    if special_form?(exp)
      eval_special_form(exp, env)
    else
      fun = _eval(car(exp), env)
      args = eval_list(cdr(exp), env)
      apply(fun, args)
    end
  end
end

def eval_list(exp, env)
  exp.map{|e| _eval(e, env)}
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
    '(' + exp.map{|e| pp(e)}.join(' ') + ')'
  else 
    exp.to_s
  end
end

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

def log(message)
#  puts message
end
//}

