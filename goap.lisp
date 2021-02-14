
(define true 1)
(define false 0)

(define enemy_sighted "enemy_sighted")
(define enemy_dead "enemy_dead")
(define enemy_in_range "enemy_in_range")
(define enemy_in_close_range "enemy_in_close_range")
(define inventory_knife "inventory_knife")
(define inventory_gun "inventory_gun")
(define gun_drawn "gun_drawn")
(define gun_loaded "gun_loaded")
(define have_ammo "have_ammo")
(define knife_drawn "knife_drawn")
(define weapon_in_hand "weapon_in_hand")
(define me_dead "me_dead")

(setq goal_win (state "goal_win"))

(setq initial_state (state "initial_state"))

(setq actions (list
	;================
	(action.eff
		(action.pre (action "scoutStealthily" 250 )
		enemy_sighted false
		weapon_in_hand true
		)
		enemy_sighted true
	)
	;================
	(action.eff
		(action.pre (action "scoutRunning" 350 )
		enemy_sighted false
		weapon_in_hand true
		)
		enemy_sighted true
	)
	;================
	(action.eff 
		(action.pre (action "closeToGunRange" 2 )
		; preconditions
		enemy_sighted true
		enemy_dead false
		enemy_in_range false
		gun_loaded true
		)
		; effects
		enemy_in_range true
	)
	;================
	(action.eff 
		(action.pre (action "closeToKnifeRange" 4 )
		; preconditions
		enemy_sighted true
		enemy_dead false
		enemy_in_close_range false
		)
		; effects
		enemy_in_close_range true
	)
	;================
	(action.eff 
		(action.pre (action "loadGun" 2 )
		; preconditions
		have_ammo true
		gun_loaded false
		gun_drawn true
		)
		; effects
		gun_loaded true
		have_ammo false
	)
	;================
	(action.eff 
		(action.pre (action "drawGun" 1 )
		; preconditions
		inventory_gun true
		weapon_in_hand false
		gun_drawn false
		)
		; effects
		gun_drawn true
		weapon_in_hand true
	)
	;================
	(action.eff 
		(action.pre (action "holsterGun" 1 )
		; preconditions
		weapon_in_hand true
		gun_drawn true
		)
		; effects
		gun_drawn false
		weapon_in_hand false
	)
	;================
	(action.eff 
		(action.pre (action "drawKnife" 1 )
		; preconditions
		inventory_knife true
		weapon_in_hand false
		knife_drawn false
		)
		; effects
		knife_drawn true
		weapon_in_hand true
	)
	;================
	(action.eff 
		(action.pre (action "sheathKnife" 1 )
		; preconditions
		weapon_in_hand true
		knife_drawn true
		)
		; effects
		knife_drawn false
		weapon_in_hand false
	)
	;================
	(action.eff 
		(action.pre (action "shootEnemy" 3 )
		; preconditions
		enemy_sighted true
		enemy_dead false
		gun_drawn true
		gun_loaded true
		enemy_in_range true
		)
		; effects
		enemy_dead true
	)
	;================
	(action.eff 
		(action.pre (action "knifeEnemy" 3 )
		; preconditions
		enemy_sighted true
		enemy_dead false
		knife_drawn true
		enemy_in_close_range true
		)
		; effects
		enemy_dead true
	)
	;================
	(action.eff 
		(action.pre (action "selfDestruct" 30 )
		; preconditions
		enemy_sighted true
		enemy_dead false
		enemy_in_range true
		)
		; effects
		enemy_dead true
		me_dead true
	)
	))


(state.set goal_win 
	enemy_dead true
	me_dead false
	)
	

(state.set initial_state 
	enemy_dead false
	enemy_sighted false
	enemy_in_range false
	enemy_in_close_range false
	gun_loaded false
	gun_drawn false
	knife_drawn false
	weapon_in_hand false
	me_dead false
	have_ammo true
	inventory_knife false
	inventory_gun true
	)
	
(setq actions_planed (plan initial_state goal_win actions))

(print actions_planed)


(setq func (lambda (plans)
	(do
	(setq st-a (state.clone initial_state))
	(state.print st-a)

	(for act plans (do 
		(print "------------------------------------------")
		(print "action" act)
		(action.print act)
		(print endl "-------------state----------")
		(state.print (setq st-a (state.action st-a act)))
	 ))
	))
)







