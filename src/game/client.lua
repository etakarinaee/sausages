local client = nil

local physics = require('physics')
local chat = require('chat')

local local_id = nil
local players = {}

local platform = { x = 0.0, y = 0.0, w = 800, h = 100 }

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
    if not id then
        id, msg_type = data:match("^(%d+):(%w+):?$")
        if not id then return nil end
    end

    id = tonumber(id)

    if msg_type == "pos" then
        local x, y = payload:match("^([^,]+),(.+)$")
        return id, "pos", tonumber(x), tonumber(y)
    elseif msg_type == "nickname" then
        return id, "nickname", payload
    elseif msg_type == "left" then
        return id, "left", nil
    elseif msg_type == "chat" then
        return id, "chat", payload
    elseif msg_type == "join" then
        return id, "join", payload
    elseif msg_type == "leave" then
        return id, "leave", payload
    end

    return nil
end

local image = core.load_texture("../test.png")

local font = core.load_font("../AdwaitaSans-Regular.ttf", 38, {32, 128})
local_nickname = os.getenv("SAUSAGES_NICKNAME") or "Player"

local softbody = 0
local softbody_two = 0;
local softbody_ground = 0;

function game_init()
    local ip = os.getenv("SAUSAGES_IP") or "127.0.0.1"
    local port = tonumber(os.getenv("SAUSAGES_PORT")) or 7777

    core.voice.init()
    core.voice.volume(1.0)

    client = core.client.new(ip, port)
    core.print("connecting to " .. ip .. ":" .. port)

    --softbody = core.create_softbody({310, 400}, {10, 10}, {0.8, 0.3, 0.0})
    softbody_two = core.create_softbody({-100, 400}, {5, 10}, {0.1, 0.0, 1.0})
    --softbody_ground = core.create_softbody({0, 0}, {25, 4}, {0.3, 0.7, 0.3})
end

local tick_rate = 1.0 / 120.0
local accumulator = 0.0

function game_update(delta_time)
    core.voice.transmit(core.key_down(key.v))
    chat.update()

   -- core.softbody_apply_force(softbody, {0, -1200})
    core.softbody_apply_force(softbody_two, {0, -1200})
    --core.update_softbody(softbody, delta_time, {0.8, 0.3, 0.0})
    core.update_softbody(softbody_two, delta_time, {0.1, 0.0, 1.0})
    --core.update_softbody(softbody_ground, delta_time, {0.3, 0.7, 0.3}) 
   -- core.softbody_check_coll({softbody, softbody_two, softbody_ground})

    local msg = chat.poll_outgoing()
    while msg do
        client:send("chat:" .. msg)
        msg = chat.poll_outgoing()
    end

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

            if id then
                if msg_type == "pos" and id ~= local_id then
                    if not players[id] then
                        players[id] = new_player()
                    end
                    players[id].x = a
                    players[id].y = b
                elseif msg_type == "nickname" and id ~= local_id then
                    if not players[id] then
                        players[id] = new_player(a)
                    else
                        players[id].nickname = a
                    end
                elseif msg_type == "left" and id ~= local_id then
                    local name = players[id] and players[id].nickname or ("Player" .. id)
                    chat.send(name .. " left the game", {1, 1, 0})
                    players[id] = nil
                elseif msg_type == "join" then
                    chat.send(a .. " joined the game", {1, 1, 0})
                elseif msg_type == "chat" then
                    if id == 65535 then
                        chat.send(a, {1, 1, 0})
                    else
                        local name = players[id] and players[id].nickname or ("Player" .. id)
                        chat.add(name, a)
                    end
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

        if core.mouse_down(mouse.left) then
            pos = core.mouse_position()
            pos_world = core.screen_to_world({pos.x, pos.y})
            core.softbody_set_pos(softbody_two, pos_world)
        end

        if not chat.is_active() then
            if core.key_down(key.a) then
                local_player.vx = local_player.vx - speed * tick_rate
                core.softbody_apply_force(softbody_two, {-speed * 90 * tick_rate, 0})
            end
            if core.key_down(key.d) then
                local_player.vx = local_player.vx + speed * tick_rate
                core.softbody_apply_force(softbody_two, {speed * 90 * tick_rate, 0})
            end
            if core.key_just_down(key.space) then
                core.softbody_apply_velocity(softbody_two, {0, 6000})
            end 
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

    --core.draw_softbody(softbody)
    core.draw_softbody(softbody_two)
    --core.draw_softbody(softbody_ground)

    local pos = core.softbody_get_pos(softbody_two)
    core.push_circle({pos.x, pos.y}, 15, {1.0, 1.0, 1.0})

    if core.button(font, "button", {0, 0}, {300, 100}) then
        core.print("YOO")
    end

    chat.draw()
end

function game_quit()
    if client then client:close() end
    core.destroy_softbody(softbody)
end
