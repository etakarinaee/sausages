local server = nil

function game_init()
    server = core.server.new(7777, 32)
    core.print("server listening on 7777")
end

function game_update(dt)
    local ev = server:poll()
    while ev do
        if ev.type == core.net_event.connect then
            core.print("+ client " .. ev.id)
        elseif ev.type == core.net_event.disconnect then
            core.print("- client " .. ev.id)
        end
        ev = server:poll()
    end
end

function game_quit()
    if server then server:close() end
end