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
const checkTagType = (t_: TagType | {}): boolean => {
    if (!t_) {
        return false;
    }
    const t = t_ as TagType;
    return t
        && t.id !== undefined
        && t.dec_marg !== undefined
        && t.ham !== undefined
        && t.cX !== undefined
        && t.cY !== undefined
        && t.cLTx !== undefined
        && t.cLTy !== undefined
        && t.cRTx !== undefined
        && t.cRTy !== undefined
        && t.cRBx !== undefined
        && t.cRBy !== undefined
        && t.cLBx !== undefined
        && t.cLBy !== undefined;
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
const checkAirplaneStateType = (t_: AirplaneStateType | {}): boolean => {
    if (!t_) {
        return false;
    }
    const t = t_ as AirplaneStateType;
    return t
        && t.timestamp !== undefined
        && t.voltage !== undefined
        && t.high !== undefined
        && t.pitch !== undefined
        && t.roll !== undefined
        && t.yaw !== undefined
        && t.vx !== undefined
        && t.vy !== undefined
        && t.vz !== undefined;
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
const checkTagInfoType = (t: TagInfoType): boolean => {
    return t
        && t.imageX !== 0
        && t.imageY !== 0
        && t.tagInfo !== undefined
        && t.tagInfo.center !== undefined
        && t.tagInfo.list !== undefined
        && t.airplaneState !== undefined;
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

    function pythagoreanDistance(x: number, y: number): number;
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

    // TODO

    return [];
};

// translate image (0,0) from LT to LB
const y2y = (imageY: number, y: number) => {
    return imageY - y;
};

interface Vec2 {
    x: number,
    y: number,
}

interface Point2 {
    x: number,
    y: number,
}

interface PlaneInfo {
    // the vector of plane X axis in image
    xDirect: Vec2;
    // the vector of plane Z axis in image
    zDirect: Vec2;
    // Point 2 Point pair
    // cm XZ
    PlaneP: Point2;
    // pixel XY
    ImageP: Point2;
    // Plane scale of image on Plane XZ Direct
    // imgPixel[px]/planeDistance[1m]
    ScaleXZ: Vec2;
    // Plane scale in image XY Direct
    // planeDistance[mm]/imgPixel[1px]
    ScaleXY: Vec2;
}

const calcVectorLineScale = (va: Vec2, vb: Vec2) => {
    return MathEx.pythagoreanDistance(va.x, va.y) / MathEx.pythagoreanDistance(vb.x, vb.y);
};

const calcImageCenterInPlane = (imageX: number, imageY: number, planeInfo: PlaneInfo) => {
    // TODO re-calc to update planeInfo P-P-pair
    return planeInfo;
};

type ResultOutputReturnType = [
    // ok ?
    boolean,
    // x, y, z. (y means h)
    number, number, number,
];

function calc_map_position(tagInfo: TagInfoType): ResultOutputReturnType {
    console.log("tagInfo:\n", JSON.stringify(tagInfo, undefined, 4));
    if (
        !checkTagInfoType(tagInfo) ||
        !checkAirplaneStateType(tagInfo.airplaneState) ||
        !checkTagType(tagInfo.tagInfo.center)
    ) {
        return [false, 0, 0, 0];
    }
    if (
        !tagInfo.tagInfo.list.find(T => checkTagType(T))
    ) {
        return [false, 0, 0, 0];
    }
    if (tagInfo.tagInfo.list.length < 2) {
        // type : calc by tag side size
        // TODO 1
    } else if (tagInfo.tagInfo.list.length === 2) {
        const l = tagInfo.tagInfo.list;
        if (l[0].id === l[1].id) {
            // type : calc by tag side size
            // TODO 1
        } else {
            // type : calc by 2 tag and it's direction
            const relation = calcTagRelation(l[0].id, l[1].id);
            console.log("relation: ", relation);
            // TODO 2
        }
    } else if (tagInfo.tagInfo.list.length >= 3) {
        const l = tagInfo.tagInfo.list;
        if (checkIsAllInSameLine(l)) {
            // type : calc by max distance 2 tag direction
            const relation = calcTagRelation(l[0].id, l[1].id);
            console.log("relation: ", relation);
            // TODO 2
        } else {
            // type : to find last triangle
            // TODO 3 findOuterSide
        }
    }
    return [true, 0, 1, 2];
}
