
(assert 
(forall ( (x1 Int) (x2 Int))
	(exists ((y Int))(
and (
= (div x1 4) (div y 4)
) (
< (div y 3) (div x1 2)
)
))))