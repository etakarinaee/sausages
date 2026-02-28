local physics = {}

function physics.aabb_overlap(ax, ay, aw, ah, bx, by, bw, bh)
    return ax - aw/2 < bx + bw/2 and
           ax + aw/2 > bx - bw/2 and
           ay - ah/2 < by + bh/2 and
           ay + ah/2 > by - bh/2
end

return physics