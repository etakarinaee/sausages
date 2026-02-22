function game_init()
end

function game_update(delta_time)
    core.print("Hello, Word!")
    core.push_quad({0.1, 0.1}, {0.5, 0.5, 0.5});
end

function game_quit()
end
