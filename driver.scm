(import
 (scheme small)
 (srfi 144)
 (compiler ast))

;; de-structuring let
;; explode structure into constituting terms
(define ds:let-star
  (lambda (n let-list struct body)
    (if (null? let-list)
        (de-sugar body)
        (list 'ast:let (symbol->string (car let-list)) (list 'ast:pi n struct)
              (ds:let-star (+ n 1) (cdr let-list) struct body)))))


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

;; commutative prim ops like + * ...
;; these de-sugar into a tree
;; eg (+ a b c) => (+ a' (+ b' c'))
;; where terms' are de-sugared recursively
;; some care has to be taken for (+ x) => x'/
;; and (+ x y) => (+ x' y')
(define ds:pcm
  (lambda (op args)
    (cond ((null? args)       '())
          ((null? (cdr args)) (de-sugar (car args)))
          (else (list 'ast:prim op
                      `(list
                       ,(de-sugar (car args))
                       ,(ds:pcm op (cdr args))))))))

;; prim op which depend on order like / - ...
;; these desugar as (- a b c d) => (- a (+ b c d)')
;; where ' denotes de-sugaring
;; we also get a neutral element `e' for the special cases
;; (/ x) => (/ 1 x')
;; (- x) => (- 0 x')
(define ds:pnc
  (lambda (op iop e arg args)
    (if (null? args)
        (list 'ast:prim op (list e (de-sugar arg)))
        (list 'ast:prim op
              `(list
               ,(de-sugar arg)
               ,(ds:pcm iop args))))))

;; argument list: de-sugar all terms
(define ds:arg
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
    (cons 'ast:tuple `((list ,@(ds:arg ts))))))

(define ds:var                                      ;; tuple introduction
  (lambda (v)
    (list 'ast:var (symbol->string v))))

(define de-sugar
  (lambda (dsl)
    (cond
     ((list? dsl)
      (case (car dsl)
        ('tuple  (ds:tup (cdr dsl)))                     ;; a tuple
        ('let    (ds:let (cadr dsl) (caddr dsl)))        ;; a let-chain of type (let ((x0 v0) (x1 v1) ...) body)
        ('let*   (ds:let-star 0 (caadr dsl) (de-sugar (cadadr dsl)) (caddr dsl)))   ;; a destructuring let
        ('lambda (ds:lam (cadr dsl) (caddr dsl))) ;; anonymous function
        ('+      (ds:pcm "+" (cdr  dsl)))  ;; associative primop
        ('*      (ds:pcm "*" (cdr  dsl)))
        ('-      (ds:pnc "-" "+" '(ast:f64 0) (cadr dsl) (cddr dsl)))
        ('/      (ds:pnc "/" "*" '(ast:f64 1) (cadr dsl) (cddr dsl)))
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

(define-syntax define-mechanism
  (syntax-rules ()
    ((define-mechanism :name name :current code)
     (cps:gen-cxx
      (compile
       `(let ((,'name ,'code)) ,'name))))))

(display "Ih mech current\n")
(define ih-current
  '(lambda (sim mech)
    (let* ((sim_v sim_i sim_g) sim)
      (let* ((mech_m mech_gbar mech_ehcn) mech)
        (let ((i_new (+ sim_i (* mech_gbar mech_m (- sim_v mech_ehcn))))
              (g_new (+ sim_g (* mech_gbar mech_m))))
          (tuple i_new g_new))))))

(define-mechanism
  :name Ih
  :current
  (lambda (sim mech)
    (let* ((sim_v sim_i sim_g) sim)
      (let* ((mech_m mech_gbar mech_ehcn) mech)
        (let ((i_new (+ sim_i (* mech_gbar mech_m (- sim_v mech_ehcn))))
              (g_new (+ sim_g (* mech_gbar mech_m))))
          (tuple i_new g_new))))))
