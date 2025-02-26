extends Node2D

# Constants (from your common.h)
const WIDTH = 1280
const HEIGHT = 720
const MAP_WIDTH = 1920
const MAP_HEIGHT = 1080
const SPRITE_SIZE = 32
const SAND = Color(194.0/255.0, 178.0/255.0, 128.0/255.0)
const NIGHT_SAND = Color(100.0/255.0, 80.0/255.0, 50.0/255.0)
const GRAY = Color(128.0/255.0, 128.0/255.0, 128.0/255.0)
const BROWN = Color(139.0/255.0, 69.0/255.0, 19.0/255.0)
const DARK_SAND = Color(150.0/255.0, 130.0/255.0, 80.0/255.0)
const GRASS = Color(34.0/255.0, 139.0/255.0, 34.0/255.0)
const PAVEMENT = Color(200.0/255.0, 200.0/255.0, 200.0/255.0)
const BLACK = Color(0, 0, 0)
const GREEN = Color(0, 1, 0)
const WHITE = Color(1, 1, 1)

# Node references
onready var recruit = $Recruit  # AnimatedSprite
onready var di = $DI  # AnimatedSprite
onready var gear = $Gear  # Sprite
onready var camera = $Camera2D
onready var tilemap = $TileMap
onready var ui = $CanvasLayer

# Game variables
var recruit_pos = Vector2(400, 250)
var di_pos = Vector2(1200, 500)
var gear_pos = Vector2(randi() % (MAP_WIDTH - 200) + 100, randi() % (MAP_HEIGHT - 200) + 100)
var recruit_speed = 2.5
var sprint_speed = 5.0
var di_speed = 3.0
var stamina = 100.0
var max_stamina = 100.0
var stamina_drain = 0.1
var stamina_regen = 0.2
var catch_count = 0
var max_catches = 15
var catch_cooldown = 120
var last_catch_check = 0
var gear_collected = false
var running = true
var di_latched = false
var frame_count = 0
var weather_timer = 0
var day_night_cycle = 0
var latch_timer = 0
const LATCH_DURATION = 120

# Map data (converted from your C++ vectors)
var barracks = [Rect2(200, 100, 64, 64), Rect2(500, 400, 64, 64), Rect2(600, 500, 64, 64)]
var obstacle_courses = [Rect2(100, 300, 192, 48)]
var sand_pits = [Rect2(300, 300, 100, 100)]
var roads = [Rect2(800 - 16, 0, 32, MAP_HEIGHT)]
var rifle_ranges = [Rect2(600, 100, 192, 48)]
var parade_decks = [Rect2(100, 500, 192, 48)]
var chow_halls = [Rect2(600, 500, 64, 64)]
var dust_particles = []

func _ready():
	# Initialize camera
	camera.make_current()
	camera.position_smoothing_enabled = true
	
	# Set initial positions
	recruit.position = recruit_pos
	di.position = di_pos
	gear.position = gear_pos
	
	# Start animations
	recruit.play("walk")
	recruit.stop()  # Stop until moving
	di.play("yell")
	di.stop()

func _process(delta):
	if not running:
		return
	
	# Input handling
	var direction = Vector2.ZERO
	var is_sprinting = Input.is_action_pressed("sprint")  # Map "sprint" to Shift in Input Map
	if Input.is_action_pressed("ui_up"): direction.y -= 1
	if Input.is_action_pressed("ui_down"): direction.y += 1
	if Input.is_action_pressed("ui_left"): direction.x -= 1
	if Input.is_action_pressed("ui_right"): direction.x += 1
	
	if direction != Vector2.ZERO:
		direction = direction.normalized()
	
	# Movement and stamina
	var current_speed = sprint_speed if (is_sprinting and stamina > 0 and not di_latched) else recruit_speed
	if is_sprinting and stamina > 0 and not di_latched:
		stamina -= stamina_drain
	elif stamina < max_stamina and not di_latched:
		stamina += stamina_regen
	stamina = clamp(stamina, 0, max_stamina)
	
	# Move recruit (unless latched)
	var new_pos = recruit_pos
	var in_sand_pit = false
	if not di_latched:
		new_pos += direction * current_speed
		for pit in sand_pits:
			if pit.has_point(new_pos):
				in_sand_pit = true
				stamina -= stamina_drain
				new_pos = recruit_pos + direction * (current_speed / 2.0)  # Slower in sand
		for structure in (barracks + obstacle_courses + rifle_ranges + parade_decks + chow_halls):
			if structure.has_point(new_pos + Vector2(SPRITE_SIZE / 2, SPRITE_SIZE / 2)):  # Center point
				new_pos = recruit_pos  # Block movement
		recruit_pos = new_pos.clamp(Vector2.ZERO, Vector2(MAP_WIDTH - SPRITE_SIZE, MAP_HEIGHT - SPRITE_SIZE))
	
	# DI logic
	if not gear_collected:
		if di_latched:
			latch_timer += 1
			di_pos = recruit_pos
			if latch_timer >= LATCH_DURATION or (stamina <= 0 and latch_timer >= LATCH_DURATION / 2):
				di_latched = false
				latch_timer = 0
				di_pos += Vector2(randi() % 201 - 100, randi() % 201 - 100)
				print("Escaped the DI automatically!")
		else:
			var di_direction = (recruit_pos - di_pos).normalized()
			var new_di_pos = di_pos + di_direction * di_speed
			for pit in sand_pits:
				if pit.has_point(new_di_pos):
					new_di_pos = di_pos + di_direction * (di_speed / 2.0)
			for structure in (barracks + obstacle_courses + rifle_ranges + parade_decks + chow_halls):
				if structure.has_point(new_di_pos + Vector2(SPRITE_SIZE / 2, SPRITE_SIZE / 2)):
					new_di_pos = di_pos
			di_pos = new_di_pos.clamp(Vector2.ZERO, Vector2(MAP_WIDTH - SPRITE_SIZE, MAP_HEIGHT - SPRITE_SIZE))
			
			# Catch check
			if recruit_pos.distance_to(di_pos) < 20 and not di_latched and last_catch_check >= catch_cooldown:
				di_latched = true
				latch_timer = 0
				catch_count += 1
				last_catch_check = 0
				print("DI caught you! Times caught: ", catch_count, "/15")
				if catch_count >= max_catches:
					running = false
					print("DI won! Game Over!")
	
	# Gear collection
	if recruit_pos.distance_to(gear_pos) < 20 and not gear_collected:
		gear_collected = true
		gear.visible = false
		print("Gear collected!")
		if di_latched:
			di_latched = false
			latch_timer = 0
			di_pos += Vector2(randi() % 201 - 100, randi() % 201 - 100)
	
	# Update positions
	recruit.position = recruit_pos
	di.position = di_pos
	
	# Camera follow
	camera.position = recruit_pos
	
	# Animation control
	if direction != Vector2.ZERO and not di_latched:
		if not recruit.is_playing():
			recruit.play("walk")
	else:
		recruit.stop()
	if not gear_collected and not di_latched:
		if not di.is_playing():
			di.play("yell")
	else:
		di.stop()
	
	# Weather and day/night
	frame_count += 1
	day_night_cycle = (frame_count / 300) % 2
	modulate = SAND if day_night_cycle == 0 else NIGHT_SAND  # Tint entire scene
	if frame_count % 600 == 0:
		weather_timer = 120
	if weather_timer > 0:
		weather_timer -= 1
		if randi() % 5 == 0:
			dust_particles.append(Vector2(recruit_pos.x + SPRITE_SIZE / 2, recruit_pos.y + SPRITE_SIZE / 2))
	
	# Update dust particles
	for i in range(dust_particles.size() - 1, -1, -1):
		dust_particles[i].y += 1
		if (dust_particles[i].y > camera.position.y + HEIGHT or 
			dust_particles[i].x < camera.position.x or 
			dust_particles[i].x > camera.position.x + WIDTH):
			dust_particles.remove(i)
	
	# Update UI
	update()
	
	last_catch_check += 1

func _input(event):
	if event.is_action_pressed("ui_cancel"):  # ESC to quit
		running = false
		get_tree().quit()
	if di_latched and event.is_action_pressed("escape_di"):  # Space to escape (map in Input Map)
		var escape_chance = min(0.9, stamina / max_stamina)
		if randf() < escape_chance:
			di_latched = false
			latch_timer = 0
			di_pos += Vector2(randi() % 201 - 100, randi() % 201 - 100)
			print("Escaped the DI!")
		else:
			stamina -= 15.0
			print("Failed to escape!")

func _draw():
	# Stamina bar
	draw_rect(Rect2(10, 10, 200, 20), BLACK)
	draw_rect(Rect2(10, 10, 200 * (stamina / max_stamina), 20), GREEN)
	
	# Dust particles
	for particle in dust_particles:
		draw_rect(Rect2(particle - camera.position, Vector2(8, 8)), BROWN)
	
	# Text (simplified, add Label nodes in CanvasLayer for better UI)
	if not gear_collected and recruit_pos.distance_to(di_pos) < 100 and not di_latched:
		draw_string(preload("res://sprites/DejaVuSans.ttf"), Vector2(10, HEIGHT - 100), "You look like the monkey off Ace Ventura!", BLACK)
	if gear_collected:
		draw_string(preload("res://sprites/DejaVuSans.ttf"), Vector2(10, HEIGHT - 60), "Mission Complete: Gear Secured!", BLACK)
	draw_string(preload("res://sprites/DejaVuSans.ttf"), Vector2(10, 40), "Times Caught: %d/15" % catch_count, BLACK)
	if di_latched:
		draw_string(preload("res://sprites/DejaVuSans.ttf"), Vector2(WIDTH / 2 - 50, HEIGHT - 150), "Press Space to Escape!", WHITE)
