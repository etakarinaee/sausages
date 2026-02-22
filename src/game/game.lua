

local image = core.load_texture("../test.png")
local angle = 0.0

local pos_x = 0.5 
local pos_y = 0.5

function game_init()
end

function game_update(delta_time)
    if core.mouse_down(mouse.left) then 
        local dim = core.get_screen_dimensions()
        local pos = core.mouse_pos()
    
        local x = (pos.x / dim.width) * 2.0 - 1.0 
        local y = -((pos.y / dim.height) * 2.0 - 1.0)

        core.push_quad({x, y}, {0.5, 0.5, 0.5}, image);
    end

    if core.key_pressed(key.r) then 
        angle = angle + 1.0
        core.print("Angle:" .. tostring(angle))
    end

    if core.key_pressed(key.w) then 
        pos_y = pos_y + 1.0 * delta_time
    end
    if core.key_pressed(key.s) then 
        pos_y = pos_y - 1.0 * delta_time
    end
    if core.key_pressed(key.d) then 
        pos_x = pos_x + 1.0 * delta_time
    end
    if core.key_pressed(key.a) then 
        pos_x = pos_x - 1.0 * delta_time
    end

    core.push_quad_ex({pos_x, pos_y}, {0.5, 0.5, 0.5}, image, 0.2, angle);
end

function game_quit()
end
