

local image = core.load_texture("../test.png")

function game_init()
end

function game_update(delta_time)
    if core.mouse_down(mouse.left) == 1 then 
        local pos = core.mouse_pos()
        core.push_quad({0.1, 0.1}, {0.5, 0.5, 0.5}, image);
    end
end

function game_quit()
end
