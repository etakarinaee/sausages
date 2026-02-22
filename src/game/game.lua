

local image = core.load_texture("../test.png")
local angle = 0.0

function game_init()
end

function game_update(delta_time)
    if core.mouse_down(mouse.left) then 
        local dim = core.get_screen_dimensions()
        local pos = core.mouse_pos()
    
        local x = (pos.x / dim.width) * 2.0 - 1.0
        local y = -((pos.y / dim.height) * 2.0 - 1.0)

        core.push_quad_ex({x, y}, {0.5, 0.5, 0.5}, image, 0.2, angle);
    end

    if core.key_pressed(key.r) then 
        angle = angle + 1.0
        core.print("Angle:" .. tostring(angle))
    end
end

function game_quit()
end
