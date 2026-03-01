local server = nil
local clients = {}  -- { [id] = { nickname = "name" } }

function game_init()
    server = core.server.new(os.getenv("SAUSAGES_IP") or "127.0.0.1", 7777, 32)
    core.print("server listening on 7777")
end

function game_update(dt)
    local ev = server:poll()
    while ev do
        if ev.type == core.net_event.connect then
            clients[ev.id] = { nickname = "Player" .. ev.id }
            core.print(ev.id .. " joined the game")
        elseif ev.type == core.net_event.disconnect then
            core.print(clients[ev.id].nickname .. " left the game")
            server:broadcast(ev.id .. ":left:")
            clients[ev.id] = nil
        elseif ev.type == core.net_event.data then
            local msg_type, payload = ev.data:match("^(%w+):(.+)$")

            if msg_type == "nickname" then
                clients[ev.id].nickname = payload
                core.print(ev.id .. " set nickname to " .. payload)
                server:broadcast(ev.id .. ":nickname:" .. payload)
            elseif msg_type == "pos" then
                for id, _ in pairs(clients) do
                    server:send(id, ev.id .. ":pos:" .. payload)
                end
            end
        end
        ev = server:poll()
    end
end

function game_quit()
    if server then server:close() end
end
