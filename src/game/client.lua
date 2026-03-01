local client = nil
local physics = require('physics')

local local_id = nil
local local_nickname = os.getenv("SAUSAGES_NICKNAME") or "Player"
local players = {}

local platform = { x = 0.0, y = -0.5, w = 800, h = 100 }

local gravity = -300.0
local friction = 0.88
local player_w = 30
local player_h = 30
local speed = 1200.0

local function new_player(nickname)
    return {
        x = 0.0,
        y = 0.5,
        vx = 0.0,
        vy = 0.0,
        nickname = nickname or "Unknown",
    }
end

local function serialize_position(player)
    return string.format("pos:%.4f,%.4f", player.x, player.y)
end

local function deserialize_message(data)
    local id, msg_type, payload = data:match("^(%d+):(%w+):(.+)$")
    if not id then return nil end

    id = tonumber(id)

    if msg_type == "pos" then
        local x, y = payload:match("^([^,]+),(.+)$")
        return id, "pos", tonumber(x), tonumber(y)
    elseif msg_type == "nickname" then
        return id, "nickname", payload
    elseif msg_type == "left" then
        return id, "left", nil
    end

    return nil
end

local image = core.load_texture("../test.png")
local font = core.load_font("../AdwaitaSans-Regular.ttf", 48, {32, 128})

function game_init()
    local ip = os.getenv("SAUSAGES_IP") or "127.0.0.1"
    client = core.client.new(ip, 7777)
    core.print("connecting to " .. ip .. ":7777")
end

local tick_rate = 1.0 / 120.0
local accumulator = 0.0

function game_update(delta_time)
    local ev = client:poll()
    while ev do
        if ev.type == core.net_event.connect then
            local_id = ev.id
            players[local_id] = new_player(local_nickname)
            client:send("nickname:" .. local_nickname)
        elseif ev.type == core.net_event.disconnect then
            core.print("disconnected")
            local_id = nil
        elseif ev.type == core.net_event.data then
            local id, msg_type, a, b = deserialize_message(ev.data)

            if id and id ~= local_id then
                if msg_type == "pos" then
                    if not players[id] then
                        players[id] = new_player()
                    end
                    players[id].x = a
                    players[id].y = b
                elseif msg_type == "nickname" then
                    if not players[id] then
                        players[id] = new_player(a)
                    else
                        players[id].nickname = a
                    end
                elseif msg_type == "left" then
                    players[id] = nil
                end
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

        if local_player.y < -1000 then
            local_player.x, local_player.y, local_player.vx, local_player.vy = 0.0, 0.5, 0.0, 0.0
        end
    end

    client:send(serialize_position(local_player))

    core.push_rect({platform.x, platform.y}, {platform.w, platform.h}, {0.3, 0.7, 0.3})
    for id, player in pairs(players) do
        core.push_texture({player.x, player.y}, {player_w, player_h}, image)
        core.push_text_ex(font, player.nickname, {player.x, player.y + 10}, 25, {1.0, 1.0, 1.0}, core.anchor.center)
    end

    if core.button(font, "button", {0, 0}, {300, 100}) then
        core.print("YOO")
    end
end

function game_quit()
    if client then client:close() end
end
