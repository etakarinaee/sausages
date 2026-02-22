

local image = core.load_texture("../test.png")

function game_init()
end

function game_update(delta_time)
    if core.mouse_down(mouse.left) == 1 then 
        local dim = core.get_screen_dimensions()
        local pos = core.mouse_pos()
    
        local x = (pos.x / dim.width) * 2.0 - 1.0
        local y = -((pos.y / dim.height) * 2.0 - 1.0)

        core.push_quad({x, y}, {0.5, 0.5, 0.5}, image);
    end
end

function game_quit()
end
