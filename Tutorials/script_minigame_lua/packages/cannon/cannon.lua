implements("ot.gameob_script")

function ot.gameob_script:init_chassis(params)
    return {};
end

function ot.gameob_script:initialize()
    --Set fps camera position
    self:set_fps_camera_pos({x = 0, y = 0, z = 5})
end
