#!/usr/bin/env ruby
# $DEBUG  = true
# $DEBUG_SCHEME_IN_SCHEME = true

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
  :print => [:prim, lambda{|exp| puts(exp.inspect) if $DEBUG_SCHEME_IN_SCHEME
               exp}]
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
# written by dosaka
  s = s.clone
  s.gsub!(/'([a-zA-Z_\+\-\*><=][0-9a-zA-Z_\+\-=!*]*)/, "(quote \\1)")
  while s.sub!(/'([^()]+|(?<pa>\((\?:\s|[^()]+|\g<pa>)*\)))/) {|m|
      m.sub(/^'(.*)$/, "(quote \\1)")
    }
  end
  s
end

def parse(exp)
  program = macro_quote(exp.gsub(/\n/, ' ').strip()).
    # gsub(/'([^()]+|(?<quote>\((\?:\s|[^()]+|\g<quote>)*\)))/, 
    #      '(quote \k<quote>)').
    gsub(/[a-zA-Z_\+\-\*><=][0-9a-zA-Z_\+\-=!*]*/, ':\\0').
    gsub(/\s+/, ', ').
    gsub(/\(/, '[').
    gsub(/\)/, ']')
  log(program)
  eval(program)
end

def apply(fun, args)
  log "apply fun:#{fun}, args:#{pp(args)}"
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
  #    log "lookup_var: var:#{var}, env: #{env}"
  alist = env.find{|alist| alist.key?(var)}
  # log "lookup_var: var:#{var}, val:#{alist.nil? ? nil : alist[var]}"
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
  log("eval :exp: #{pp(exp)}, env:#{pp(env)}")
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
      # puts "eval_exp_line:#{line}"
      # puts "eval_exp:#{parse(line)}"
      val = _eval(parse(line), $global_env)
    rescue Exception => e
      puts e.to_s
      redo
    end
    puts pp(val)
  end
end

def log(message)
  if ($DEBUG)
    puts message
  end
end

def assert(tested, expected)
  if expected != tested
    raise "test failed. expencted:'#{expected}', but test:'#{tested}'"
  end
end

$programs_expects =
  [
   # test environment
   [[[:lambda, [:x],
      [:+, 
       [[:lambda, [:x], :x], 2],
       :x]], 
     1],
    3],
   # test let
   [[:let, [[:x, 2], [:y, 3]], [:+, :x, :y]],
    5],
   [[:let, [[:x, 2] , [:y, 3]], [[:lambda, [:x, :y], [:+, :x, :y]], :x, :y]],
     5],
   [[:let, [[:add, [:lambda, [:x, :y], [:+, :x, :y]]]], [:add, 2, 3]],
    5],
   # test if
   [[:if, [:>, 3, 2], 1, 0],
    1],
   # test letrec
   [[:letrec, 
     [[:fact,
       [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]], 
     [:fact, 3]], 
    6],
   # test cond 
   [[:cond, 
    [[:<, 2, 1], 0],
    [[:<, 2, 1], 1],
    [:else, 1]], 
    1],
   # test define
   [[:define, [:length, :list], 
    [:if, [:null?, :list], 0, 
     [:+, [:length, [:cdr, :list]], 1]]], 
    nil],
   [[:length, [:list, 1, 2]],
    2],
   [[:define, [:id, :x], :x],
    nil],
   [[:id, 3],
    3],
   [[:define, :x, [:lambda, [:x], :x]],
    nil],
   [[:x, 3],
    3],
   [[:define, :x, 5],
    nil],
   [:x,
    5],
   # test set!
   [[:let, [[:x, 1]],
     [:let, [[:dummy, [:set!, :x, 2]]],
      :x]], 2],
   # test list
   [[:list, 1],
    [1]],
   # test repl
   [parse('(define (length list) (if  (null? list) 0 (+ (length (cdr list)) 1)))'),
    nil],
   [parse('(length (list 1 2 3))'), 
    3],
   [parse('(letrec ((fact (lambda (n) (if (< n 1) 1 (* n (fact (- n 1))))))) (fact 3))'),
    6],
   [parse('(let ((x 1)) (let ((dummy (set! x 2))) x))'),
    2],
   # test fixed point
   # fact(0) = 1
   [[:let, 
     [[:fact,
       [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]], 
     [:fact, 0]], 1],
   # fact(1) = 1
   [[:let, 
     [[:fact,
       [:lambda, [:n], 
        [:if, [:<, :n, 1], 1, 
         [:*, :n, 
          [:let, 
           [[:fact,
             [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]],
           [:fact, [:-, :n, 1]]]]]]]], 
     [:fact, 1]], 1],
   # fact(2) = 2
   [[:let, 
     [[:fact,
       [:lambda, [:n], 
        [:if, [:<, :n, 1], 1, 
         [:*, :n, 
          [:let, 
           [[:fact,
             [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]],
           [:let, 
            [[:fact,
              [:lambda, [:n], [:if, [:<, :n, 1], 1, [:*, :n, [:fact, [:-, :n, 1]]]]]]],
            
            [:fact, [:-, :n, 1]]]]]]]]], 
     [:fact, 2]], 2],
   # closure
   [parse(
<<EOS
(define (makecounter)
  (let ((count 0))
    (lambda ()
      (let ((dummy (set! count (+ count 1))))
	count))))
EOS
), nil],
   [parse('(define inc (makecounter))'), nil],
   [parse('(inc)'), 1],
   [parse('(inc)'), 2],
  ]

def test
  $programs_expects.each do |exp, expect| 
    val = _eval(exp, $global_env)
    log("test: exp:#{pp(exp)}, expect:#{pp(expect)}, result:#{pp(val)}")
    assert(val, expect)
  end
end

test

if $0 == __FILE__
  repl
end
