local chat = {}

local messages = {}
local buffer = ''
local active = false

function chat.update()
    if core.key_just_down(key.t) and not active then
        active = true
        buffer = ""
        return
    end

    if not active then
        return
    end

    local text = core.get_text_input()
    if text then
        buffer = buffer .. text
    end

    if core.key_just_down(key.backspace) and #buffer > 0 then
        buffer = buffer:sub(1, -2)
    end

    if core.key_just_down(key.enter) then
        if #buffer > 0 then
            chat.add(local_nickname, buffer)
        end
        buffer = ""
        active = false
    end

    if core.key_just_down(key.escape) then
        buffer = ""
        active = false
    end
end

function chat.draw()
    if not font then
        return
    end

    local screen = core.get_screen_dimensions()
    local x = -screen.width / 2 + 10
    local y = -screen.height / 2 + 10

    if active then
        core.push_text(font, buffer .. "_", {x, y}, 18, {1, 1, 1})
        y = y + 22
    end

    for i = #messages, math.max(1, #messages - 9), -1 do
        core.push_text(font, messages[i], {x, y}, 18, {1, 1, 1})
        y = y + 22
    end
end

function chat.is_active()
    return active
end

function chat.add(name, message)
    table.insert(messages, "<" .. name .. "> " .. message)
end


return chat