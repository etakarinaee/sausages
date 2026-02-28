local server = nil
local clients = {}

function game_init()
    server = core.server.new(7777, 32)
    core.print("server listening on 7777")
end

function game_update(dt)
    local ev = server:poll()
    while ev do
        if ev.type == core.net_event.connect then
            clients[ev.id] = true
            core.print(ev.id .. " joined the game")
        elseif ev.type == core.net_event.disconnect then
            core.print(ev.id .. " left the game")
            clients[ev.id] = nil
            server:broadcast(ev.id .. ":-999,-999")
        elseif ev.type == core.net_event.data then
            local message = ev.id .. ":" .. ev.data
            for id, _ in pairs(clients) do
                server:send(id, message)
            end
        end
        ev = server:poll()
    end
end

function game_quit()
    if server then server:close() end
end