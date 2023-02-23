export enum OwlCmdEnum {
    ping = 0,
    emergencyStop = 120,
    unlock = 92,

    calibrate = 90,
    break = 10,
    takeoff = 11,
    land = 12,
    move = 13,
    rotate = 14,
    keep = 15,
    goto = 16,

    led = 17,
    high = 18,
    speed = 19,
    flyMode = 20,

}

export enum OwlCmdRotateEnum {
    cw = 1,
    ccw = 2,
}

export enum OwlCmdMoveEnum {
    up = 1,
    down = 2,
    left = 3,
    right = 4,
    forward = 5,
    back = 6,
}

