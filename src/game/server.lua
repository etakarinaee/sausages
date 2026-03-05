local server = nil
local clients = {}

local server_id = 65535

function game_init()
    server = core.server.new(os.getenv("SAUSAGES_IP") or "127.0.0.1", 7777, 32)
    core.print("server listening on 7777")
end

function game_update(dt)
    local line = core.read_stdin()
    if line and #line > 0 then
        core.print(line)
        server:broadcast(server_id .. ":chat:" .. line)
    end

    local ev = server:poll()
    while ev do
        if ev.type == core.net_event.connect then
            clients[ev.id] = { nickname = "Player" .. ev.id, joined = false }

            for id, client in pairs(clients) do
                if id ~= ev.id then
                    server:send(ev.id, id .. ":nickname:" .. client.nickname)
                end
            end

        elseif ev.type == core.net_event.disconnect then
            local nickname = clients[ev.id].nickname
            core.print(nickname .. " left the game")
            server:broadcast(ev.id .. ":left:")
            clients[ev.id] = nil

        elseif ev.type == core.net_event.data then
            local msg_type, payload = ev.data:match("^(%w+):(.+)$")

            if msg_type == "nickname" then
                local is_join = not clients[ev.id].joined
                clients[ev.id].nickname = payload
                clients[ev.id].joined = true
                server:broadcast(ev.id .. ":nickname:" .. payload)

                if is_join then
                    core.print(payload .. " joined the game")
                    for id, _ in pairs(clients) do
                        server:send(id, ev.id .. ":join:" .. payload)
                    end
                end
            elseif msg_type == "pos" then
                for id, _ in pairs(clients) do
                    if id ~= ev.id then
                        server:send(id, ev.id .. ":pos:" .. payload)
                    end
                end
            elseif msg_type == "chat" then
                core.print("<" .. clients[ev.id].nickname .. "> " .. payload)
                for id, _ in pairs(clients) do
                    if id ~= ev.id then
                        server:send(id, ev.id .. ":chat:" .. payload)
                    end
                end
            end
        end
        ev = server:poll()
    end
end

function game_quit()
    if server then server:close() end
end