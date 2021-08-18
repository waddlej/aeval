(declare-var a (Array Int Int))
(declare-var a1 (Array Int Int))
(declare-var b (Array Int Int))
(declare-var i Int)
(declare-var i1 Int)
(declare-var N Int)

(declare-rel inv ((Array Int Int) (Array Int Int) Int Int))
(declare-rel fail ())

(rule (inv a b 0 N))

(rule (=> (and (inv a b i N) (< i N)
    (= (ite (< (select b i) N) (store a i i) (store a i (+ i (select b i)))) a1)
    (= i1 (+ i 1)))
  (inv a1 b i1 N)))

(rule (=> (and (inv a b i N) (>= i N) (<= 0 i1) (< i1 N)
  (not (>= (select a i1) 0))) fail))

(query fail)
