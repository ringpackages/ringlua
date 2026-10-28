/*
	LuaLib Demo 19 - Game scripting
*/

load "lualib.ring"

# -------------------------------------------------------
# Demo 19: Game scripting
# -------------------------------------------------------
? "Demo 19: Game scripting..."

oLua = new LuaState

# -------------------------------------------------------
# Game world written in Lua
# -------------------------------------------------------
oLua.doString('
	entities = {}
	next_id = 1

	function createEntity(name, x, y, health, attack)
		local e = {
			id = next_id,
			name = name,
			x = x, y = y,
			health = health,
			max_health = health,
			attack = attack,
			alive = true,
			inventory = {},
			level = 1,
			xp = 0
		}
		entities[next_id] = e
		next_id = next_id + 1
		return e.id
	end

	function moveEntity(id, dx, dy)
		local e = entities[id]
		if e and e.alive then
			e.x = e.x + dx
			e.y = e.y + dy
			return true
		end
		return false
	end

	function attackEntity(attacker_id, target_id)
		local atk = entities[attacker_id]
		local def = entities[target_id]
		if not atk or not def or not atk.alive or not def.alive then
			return "Invalid combat"
		end

		-- Damage with some variance
		local damage = math.max(1, atk.attack + math.random(-2, 2))
		def.health = def.health - damage

		local msg = atk.name .. " attacks " .. def.name ..
			" for " .. damage .. " damage!"

		if def.health <= 0 then
			def.health = 0
			def.alive = false
			atk.xp = atk.xp + 50
			msg = msg .. "\n" .. def.name .. " is defeated!"
			if atk.xp >= atk.level * 100 then
				atk.level = atk.level + 1
				atk.attack = atk.attack + 2
				atk.max_health = atk.max_health + 10
				atk.health = atk.max_health
				msg = msg .. "\n" .. atk.name .. " leveled up to " .. atk.level .. "!"
			end
		else
			msg = msg .. " (" .. def.name .. " HP: " .. def.health ..
				"/" .. def.max_health .. ")"
		end

		return msg
	end

	function isAlive(id)
		return entities[id] ~= nil and entities[id].alive
	end

	function getEntityInfo(id)
		local e = entities[id]
		if not e then return nil end
		return {
			name = e.name,
			x = e.x, y = e.y,
			health = e.health,
			max_health = e.max_health,
			attack = e.attack,
			level = e.level,
			xp = e.xp,
			items = #e.inventory
		}
	end

	function healEntity(id, amount)
		local e = entities[id]
		if e and e.alive then
			e.health = math.min(e.max_health, e.health + amount)
			return e.name .. " healed for " .. amount ..
				" (HP: " .. e.health .. "/" .. e.max_health .. ")"
		end
		return "Cannot heal"
	end

	function addItem(id, item_name)
		local e = entities[id]
		if e then
			e.inventory[#e.inventory + 1] = item_name
			return e.name .. " obtained: " .. item_name
		end
		return "Entity not found"
	end

	math.randomseed(os.time())
')

# -------------------------------------------------------
# Create the entities from Ring
# -------------------------------------------------------
? "--- Creating Game World ---"

nHeroId = oLua.callFunction("createEntity", ["Hero", 0, 0, 100, 15])[1]
? "Created Hero (ID: " + nHeroId + ")"

nGoblinId = oLua.callFunction("createEntity", ["Goblin", 5, 3, 30, 8])[1]
? "Created Goblin (ID: " + nGoblinId + ")"

nDragonId = oLua.callFunction("createEntity", ["Dragon", 10, 10, 200, 25])[1]
? "Created Dragon (ID: " + nDragonId + ")"
? ""

# -------------------------------------------------------
# Game simulation
# -------------------------------------------------------
? "--- Game Simulation ---"
? ""

? "[Hero moves towards Goblin]"
oLua.callFunction("moveEntity", [nHeroId, 3, 2])

? oLua.callFunction("addItem", [nHeroId, "Iron Sword"])[1]
? oLua.callFunction("addItem", [nHeroId, "Health Potion"])[1]
? ""

? "[Combat: Hero vs Goblin]"
for nRound = 1 to 5
	? oLua.callFunction("attackEntity", [nHeroId, nGoblinId])[1]
	if ! oLua.callFunction("isAlive", [nGoblinId])[1]
		exit
	ok
	? oLua.callFunction("attackEntity", [nGoblinId, nHeroId])[1]
next
? ""

? "[Healing]"
? oLua.callFunction("healEntity", [nHeroId, 30])[1]
? ""

? "--- Hero Status ---"
aInfo = oLua.callFunction("getEntityInfo", [nHeroId])[1]
nCount = len(aInfo)
for nItem = 1 to nCount
	? "  " + aInfo[nItem][1] + ": " + aInfo[nItem][2]
next

oLua.close()
? ""
