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


(define (append list1 list2)
  (if (null? list1)
      list2
      (cons (car list1) (append (cdr list1) list2))))

(define (reverse l)
  (if (null? l)
      '()
      (append (reverse (cdr l)) (list (car l)))))

(define (map fun list)
  (if (null? list) 
      '()
      (cons (fun (car list)) (map fun (cdr list)))))

(define (zip list1 list2)
  (if (null? list1)
      '()
      (cons (list (car list1) (car list2)) (zip (cdr list1) (cdr list2)))))

(define (fold fun accum list)
  (if (null? list)
      accum
      (fold fun (fun accum (car list)) (cdr list))))

(define (make_hash)
  (quote ()))

(define (hash_put hash key val)
  (cons (list key val) hash))

(define (hash_get hash key)
  (cond ((null? hash) nil)
	((eq? (caar hash) key) (cadar hash))
	(else (hash_get (cdr hash) key))))

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
   
(__eval (quote 1) global_env)
(__eval '(lambda (x) (+ x 1)) global_env)
(__eval '(num? 1) global_env)
(__eval '(+ 1 1) global_env)
(__eval '(- 3 1) global_env)
(__eval '((lambda (x) (+ x 1)) 2) global_env)
(__eval '((lambda (x y) (+ x y)) 1 2) global_env)
(__eval '((lambda (x) (- ((lambda (x) x) 2) x)) 1) global_env)
