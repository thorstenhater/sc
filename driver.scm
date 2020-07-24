(import
 (scheme small)
 (srfi 144)
 (compiler ast))

(define ds:let
  (lambda (let-list body)
    (if (null? let-list)                            ;; at the end of a let chain,
        (de-sugar body)                             ;; ... de-sugar body
        (list 'ast:let                              ;; for one of these (... (x y) ...) make a new (let (x y') ...)
              (symbol->string(caar let-list))       ;; where x is the same binding name
              (de-sugar (cadar let-list))    ;; and y' is the de-sugared bound term
              (ds:let (cdr let-list) body)))))      ;; then recurse into the body

(define ds:lam                                      ;; for a lambda, just de-sugar the body
  (lambda (arg-list body)
    (list 'ast:lambda
          `(list ,@(map symbol->string arg-list))
          (de-sugar body))))

(define ds:pas                                       ;; associative prim ops like + * ...
  (lambda (op args)                                  ;; these de-sugar into a tree
    (cond ((null? args)       '())                   ;; eg (+ a b c) => (+ a' (+ b' c'))
          ((null? (cdr args)) (de-sugar (car args))) ;; where terms' are de-sugared recursively
          (else (list 'ast:prim op                   ;; some care has to be taken for (+ x) => x'
                      (de-sugar (car args))          ;; and (+ x y) => (+ x' y')
                      (ds:pas op (cdr args)))))))

(define ds:pod                                      ;; prim op which depend on order like / - ...
  (lambda (op iop e arg args)                       ;; these desugar as (- a b c d) => (- a (+ b c d)')
    (if (null? args)                                ;; where ' denotes de-sugaring
        (list 'ast:prim op e (de-sugar arg))        ;; we also get a neutral element `e' for the special cases
        (list 'ast:prim op                          ;; (/ x) => (/ 1 x')
              (de-sugar arg)                        ;; (- x) => (- 0 x')
              (ds:pas iop args)))))

(define ds:arg                                      ;; argument list: de-sugar all terms
  (lambda (ts)
    (if (null? ts)
        '()
        (cons (de-sugar (car ts))
              (ds:arg (cdr ts))))))

(define ds:app                                      ;; function application
  (lambda (ts)
    (list 'ast:app (de-sugar (car ts)) (ds:arg (cdr ts)))))

(define ds:tup                                      ;; tuple introduction
  (lambda (ts)
    (cons 'ast:tuple `((list ,@(ds:arg (cdr ts)))))))

(define ds:var                                      ;; tuple introduction
  (lambda (v)
    (list 'ast:var (symbol->string v))))


(define de-sugar
  (lambda (dsl)
    (cond
     ((list? dsl)
      (case (car dsl)
        ('tuple  (ds:tup (cdr dsl)))              ;; a tuple
        ('let    (ds:let (cadr dsl) (caddr dsl))) ;; a let-chain of type (let ((x0 v0) (x1 v1) ...) body)
        ('lambda (ds:lam (cadr dsl) (caddr dsl))) ;; anonymous function
        ('+      (ds:pas (car  dsl) (cdr  dsl)))  ;; associative primop
        ('*      (ds:pas (car  dsl) (cdr  dsl)))
        ('-      (ds:pod '- '+ '(ast:f64 0) (cadr dsl) (cddr dsl)))
        ('/      (ds:pod '/ '* '(ast:f64 1) (cadr dsl) (cddr dsl)))
        (else    (ds:app dsl))))                  ;; some function application
     ((number? dsl) (list 'ast:f64 (flonum dsl)))          ;; numeric literal
     ((symbol? dsl) (ds:var dsl))          ;; a symbol
     (else 'ERROR))))

(define compile
  (lambda (src)
    (cps:prim-simplify
     (cps:prim-cse
      (cps:beta-func
       (cps:beta-cont
        (cps:dead-let
         (ast->cps
          (ast:typecheck
           (ast:alpha-convert
            (eval
             (de-sugar src))))))))))))

(display
 (de-sugar '(+ 1 2 4)))
(newline)

(display
 (de-sugar '(lambda ()
              (let
                  ((x 1)
                   (y 2)
                   (z 3))
                (* x y z)))))
(newline)

(display
 (de-sugar '(let ((foo (* x (+ a b c) z))) foo)))
(newline)

(display
 (de-sugar '(let
                ((i_new (+ sim_i (* mech_gbar mech_m (+ sim_v mech_ehcn))))
                 (g_new (+ sim_g (* mech_gbar mech_m))))
              (tuple i_new g_new))))
(newline)

(display
 (de-sugar '((lambda (x y z) (+ x y z)) 1 2 (* 2 2 2))))
(newline)

(display
 (de-sugar '(- 1 2 3 4)))
(newline)

(display
 (de-sugar '(- 1 2)))
(newline)

(display
 (de-sugar '(- 1)))
(newline)

(display
 (de-sugar '(/ 1)))
(newline)

(display "Tuple: \n")
(cps:show
 (compile '(tuple 1 2 3 4 x)))
(newline)
(display "Tuple DONE \n")

(define lambda-test
  '(lambda (y)
     (let ((x 23))
       x)))

(display
 (de-sugar lambda-test))
(newline)

(cps:show
 (compile lambda-test))
(cps:gen-cxx
 (compile lambda-test))

(newline)

(define-syntax define-mechanism
  (syntax-rules ()
    ((define-mechanism :name name :current code)
     (cps:gen-cxx
      (compile
       `(let ((,'name ,'code)) ,'name))))))

(define-mechanism
  :name Ih
  :current (lambda (x y) 42))
