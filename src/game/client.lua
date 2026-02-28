local client = nil
local physics = require('physics')

local local_id = nil
local players = {}

local platform = { x = 0.0, y = -0.5, w = 800, h = 100 }

local gravity = -2.0
local speed = 1.0
local friction = 0.88
local player_w = 30
local player_h = 30

local function new_player()
    return {
        x = 0.0,
        y = 0.5,
        vx = 0.0,
        vy = 0.0,
    }
end
-- TODO: generic serialize functions (in core?)
local function serialize_position(player)
    return string.format("%.4f,%.4f", player.x, player.y)
end

-- TODO: save as with serialize
local function deserialize_position(data)
    local id, x, y = data:match("^(%d):([^,]+),(.+)$")

    if id then
        return tonumber(id), tonumber(x), tonumber(y)
    end

    return nil
end

local image = core.load_texture("../test.png")
local font = core.load_font("../AdwaitaSans-Regular.ttf", 200, {32, 128})
local angle = 0.0

local pos_x = 0.0
local pos_y = 0.0

local speed = 300.0

function game_init()
    client = core.client.new("127.0.0.1", 7777)
end

local tick_rate = 1.0 / 120.0
local accumulator = 0.0

function game_update(delta_time)
    local ev = client:poll()
    while ev do
        if ev.type == core.net_event.connect then
            local_id = ev.id
            players[local_id] = new_player()
        elseif ev.type == core.net_event.disconnect then
            core.print("disconnected")
            local_id = nil
        elseif ev.type == core.net_event.data then
            local id, x, y = deserialize_position(ev.data)

            if id and id ~= local_id then
                if not players[id] then
                    players[id] = new_player()
                end

                players[id].x = x
                players[id].y = y
            end
        end
        ev = client:poll()
    end

    local local_player = players[local_id]
    if not local_player then
        return
    end

    accumulator = accumulator + delta_time
    if accumulator > 0.2 then accumulator = 0.2 end

    while accumulator >= tick_rate do
        accumulator = accumulator - tick_rate

        if core.key_down(key.a) then
            local_player.vx = local_player.vx - speed * tick_rate
        end
        if core.key_down(key.d) then
            local_player.vx = local_player.vx + speed * tick_rate
        end

        local_player.vy = local_player.vy + gravity * tick_rate

        local_player.x = local_player.x + local_player.vx * tick_rate
        local_player.y = local_player.y + local_player.vy * tick_rate

        local_player.vx = local_player.vx * friction

        if physics.aabb_overlap(local_player.x, local_player.y, player_w, player_h, platform.x, platform.y, platform.w, platform.h) then
            if local_player.vy <= 0 then
                local_player.y = platform.y + platform.h/2 + player_h/2
                local_player.vy = 0
            end
        end

        if local_player.y < -1.2 then
            local_player.x, local_player.y, local_player.vx, local_player.vy = 0.0, 0.5, 0.0, 0.0
        end
    end

    client:send(serialize_position(local_player))

    core.push_rect({platform.x, platform.y}, {platform.w, platform.h}, {0.3, 0.7, 0.3})
    for id, player in pairs(players) do
        core.push_texture({player.x, player.y}, {player_w, player_h}, image)
        core.push_text(font, "ID: " .. id, {player.x - 20, player.y + 10}, 25, {1.0, 1.0, 1.0})
    end
end

function game_quit()
    if client then client:close() end
end
