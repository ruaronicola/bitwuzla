(declare-fun P (Bool) Bool)
(declare-fun Q (Bool Bool Bool Bool) Bool)
(assert (forall ((x Bool)) (or (P false) (exists ((y Bool)) (and y (Q x false false false))))))
(check-sat)
