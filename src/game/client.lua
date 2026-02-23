local client = nil
local physics = require('physics')

local player = {
    x = 0.0,
    y = 0.5,

    -- velocity
    vx = 0.0,
    vy = 0.0,

    w = 0.08,
    h = 0.08,

    -- TODO: get rid of that ugly bool
    on_ground = false,
}

local platform = { x = 0.0, y = -0.5, w = 1.5, h = 0.1 }

local gravity = -2.0
local force = 1.0
local speed = 1.0
local friction = 0.88

function game_init()
    client = core.client.new("127.0.0.1", 7777)
    player.x = 0.0
    player.y = 0.5
end

function game_update(delta_time)
    local ev = client:poll()
    while ev do
        if ev.type == core.net_event.connect then
            core.print("connected, id=" .. ev.id)
            client:send("hello from " .. ev.id)
        elseif ev.type == core.net_event.disconnect then
            core.print("disconnected")
        elseif ev.type == core.net_event.data then
            core.print("got: " .. ev.data)
        end
        ev = client:poll()
    end

    player.on_ground = false

    if physics.aabb_overlap(player.x, player.y, player.w, player.h, platform.x, platform.y, platform.w, platform.h) then
        if player.vy <= 0 then
            player.y = platform.y + platform.h/2 + player.h/2
            player.vy = 0
            player.on_ground = true
        end
    end

    if core.key_down(key.a) then
        player.vx = player.vx - speed * delta_time
    end
    if core.key_down(key.d) then
        player.vx = player.vx + speed * delta_time
    end

    player.vy = player.vy + gravity * delta_time

    player.x = player.x + player.vx * delta_time
    player.y = player.y + player.vy * delta_time

    player.vx = player.vx * friction

    if player.y < -1.2 then
        player.x, player.y, player.vx, player.vy = 0.0, 0.5, 0.0, 0.0
    end

    core.push_quad({platform.x, platform.y}, platform.w, platform.h, {0.3, 0.7, 0.3}, -1);

    core.push_quad({player.x, player.y}, player.w, player.h, {0.9, 0.2, 0.2}, -1)
end

function game_quit()
    if client then client:close() end
end