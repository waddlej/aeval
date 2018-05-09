(declare-rel FUN (Int Int))
(declare-rel SAD (Int Int))
(declare-rel WEE (Int Int))
(declare-var k Int)
(declare-var k1 Int)
(declare-var j Int)
(declare-var j1 Int)

(declare-rel fail ())

(rule (=> (and (= k 0) (= j 0)) (FUN k j)))

(rule (=> 
    (and 
        (FUN k j)
        (< j 1000)
        (= k1 (+ k 1))
        (= j1 (+ j 1))
    )
    (FUN k1 j1)
  )
)

(rule (=> (and (FUN k j) (>= j 1000) (= k1 k) (= j1 0)) (SAD k1 j1)))

(rule (=>
    (and
        (SAD k j)
        (< j 1000)
        (= k1 (+ k 1))
        (= j1 (+ j 1))
    )
    (SAD k1 j1)
  )
)

(rule (=> (and (SAD k j) (>= j 1000) (= k1 k) (= j1 0)) (WEE k1 j1)))

(rule (=>
    (and
        (WEE k j)
        (< j 1000)
        (= k1 (+ k 1))
        (= j1 (+ j 1))
    )
    (WEE k1 j1)
  )
)

(rule (=> (and (WEE k j) (>= j 1000) (< k 3000)) fail))

(query fail)