local client = nil

local image = core.load_texture("../test.png")
local angle = 0.0

local pos_x = 0.0
local pos_y = 0.0

local speed = 300.0

function game_init()
    client = core.client.new("127.0.0.1", 7777)
end

function game_update(dt)
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

        if core.mouse_down(mouse.left) then 
        local dim = core.get_screen_dimensions()
        local pos = core.mouse_pos()
    
        core.push_quad({pos.x - (dim.width * 0.5), -pos.y + (dim.height * 0.5)}, {0.5, 0.5, 0.5}, image);
    end

    if core.key_pressed(key.r) then 
        angle = angle + 1.0
        core.print("Angle:" .. tostring(angle))
    end

    if core.key_pressed(key.w) then 
        pos_y = pos_y + speed * dt
    end
    if core.key_pressed(key.s) then 
        pos_y = pos_y - speed * dt
    end
    if core.key_pressed(key.d) then 
        pos_x = pos_x + speed * dt
    end
    if core.key_pressed(key.a) then 
        pos_x = pos_x - speed * dt
    end
    
    core.push_quad_ex({pos_x, pos_y}, {0.5, 0.5, 0.5}, image, 300.0, angle);

end

function game_quit()
    if client then client:close() end
end