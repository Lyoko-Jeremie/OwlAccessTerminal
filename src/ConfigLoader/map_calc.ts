type TagType = {
    id: number,
    dec_marg: number,
    ham: number,
    cX: number,
    cY: number,
    cLTx: number,
    cLTy: number,
    cRTx: number,
    cRTy: number,
    cRBx: number,
    cRBy: number,
    cLBx: number,
    cLBy: number,
};
type AirplaneStateType = {
    timestamp: number,
    voltage: number,
    high: number,
    pitch: number,
    roll: number,
    yaw: number,
    vx: number,
    vy: number,
    vz: number,
};
type TagInfoType = {
    imageX: number | 0,
    imageY: number | 0,
    tagInfo: {
        center: TagType | {},
        list: TagType[],
    },
    airplaneState: AirplaneStateType | {},
};

// ============ map info ============
const MapX = 10;
const MapZ = 20;
// cm
const DistanceX = 50;
// cm
const DistanceZ = 50;
// cm
const SizeX = 10;
// cm
const SizeZ = 10;
// ============ map info ============

declare module MathEx {
    function degToRad(deg: number): number;

    function radToDeg(rad: number): number;

    function randomInt2(min: number, max: number): number;

    function distance(p1x: number, p1y: number, p2x: number, p2y: number): number;

    function distanceFast(p1x: number, p1y: number, p2x: number, p2y: number): number;
}

const calcTagPosition = (t: number): { x: number, y: number } => {
    const row = Math.floor(t / MapX);
    const col = t - row * MapX;
    return {x: col * DistanceX, y: row * DistanceZ};
};

const calcTagRelation = (t1: number, t2: number): { distance: number, rad: number, deg: number } => {
    const p1 = calcTagPosition(t1);
    const p2 = calcTagPosition(t2);
    const d = Math.atan2(p1.y - p2.y, p1.x - p2.x);
    return {
        // distance: Math.sqrt(Math.pow(Math.abs(p1.x - p2.x), 2) + Math.pow(Math.abs(p1.y - p2.y), 2)),
        distance: MathEx.distance(p1.x, p1.y, p2.x, p2.y),
        rad: d,
        deg: MathEx.radToDeg(d),
    };
};

const checkIsAllInSameLine = (list: TagType[]): boolean => {
    const lp = list.map(T => calcTagPosition(T.id));
    if (lp.length > 1) {
        const n = lp.find(T => T.x !== lp[0].x && T.y !== lp[0].y);
        return n !== undefined;
    }
    return false;
};

const calcOuterRect = (listPos: { x: number, y: number }[]): { xL: number, xU: number, zL: number, zU: number } => {
    const s = {
        xL: Number.MAX_SAFE_INTEGER, xU: Number.MIN_SAFE_INTEGER,
        zL: Number.MAX_SAFE_INTEGER, zU: Number.MIN_SAFE_INTEGER
    };
    for (let i = 0; i < listPos.length; i++) {
        const a = listPos[i];
        if (a.x < s.xL) {
            s.xL = a.x;
        }
        if (a.y < s.zL) {
            s.zL = a.y;
        }
        if (a.x > s.xU) {
            s.xU = a.x;
        }
        if (a.y > s.zU) {
            s.zU = a.y;
        }
    }
    return s;
};
const findOuterSide = (list: TagType[]): TagType[] => {
    const listPos = list.map(T => {
        const n = calcTagPosition(T.id) as { x: number, y: number, tag: TagType };
        n.tag = T;
        return n;
    });
    const outerRect = calcOuterRect(listPos);
    // find out item
    const oit: { xL: number [], xU: number[], zL: number[], zU: number[] } = {xL: [], xU: [], zL: [], zU: []};
    for (let i = 0; i < listPos.length; i++) {
        const n = listPos[i];
        if (n.x === outerRect.xL) {
            oit.xL.push(i);
        }
        if (n.x === outerRect.xU) {
            oit.xU.push(i);
        }
        if (n.y === outerRect.zL) {
            oit.zL.push(i);
        }
        if (n.y === outerRect.zU) {
            oit.zU.push(i);
        }
    }
    const center = {x: (outerRect.xU + outerRect.xL) / 2, y: (outerRect.zU + outerRect.zL) / 2};

    return [];
};

type ResultOutputReturnType = [
    // ok ?
    boolean,
    // x, y, z. (y means h)
    number, number, number,
];

function calc_map_position(tagInfo: TagInfoType): ResultOutputReturnType {
    console.log("tagInfo:\n", JSON.stringify(tagInfo, undefined, 4));
    if (tagInfo.tagInfo.list.length < 2) {
        // type : calc by tag side size
    } else if (tagInfo.tagInfo.list.length === 2) {
        const l = tagInfo.tagInfo.list;
        if (l[0].id === l[1].id) {
            // type : calc by tag side size
        } else {
            // type : calc by 2 tag and it's direction
            const relation = calcTagRelation(l[0].id, l[1].id);
            console.log("relation: ", relation);
        }
    } else if (tagInfo.tagInfo.list.length >= 3) {
        const l = tagInfo.tagInfo.list;
        if (checkIsAllInSameLine(l)) {
            // type : calc by max distance 2 tag direction
            const relation = calcTagRelation(l[0].id, l[1].id);
            console.log("relation: ", relation);
        } else {
            // type : to find last triangle
        }
    }
    return [true, 0, 1, 2];
}
