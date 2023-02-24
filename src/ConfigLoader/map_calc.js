"use strict";
const checkTagType = (t_) => {
    if (!t_) {
        return false;
    }
    const t = t_;
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
const checkAirplaneStateType = (t_) => {
    if (!t_) {
        return false;
    }
    const t = t_;
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
const checkTagInfoType = (t) => {
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
const calcTagPosition = (t) => {
    const row = Math.floor(t / MapX);
    const col = t - row * MapX;
    return { x: col * DistanceX, y: row * DistanceZ };
};
const calcTagRelation = (t1, t2) => {
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
const checkIsAllInSameLine = (list) => {
    const lp = list.map(T => calcTagPosition(T.id));
    if (lp.length > 1) {
        const n = lp.find(T => T.x !== lp[0].x && T.y !== lp[0].y);
        return n !== undefined;
    }
    return false;
};
const calcOuterRect = (listPos) => {
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
const findOuterSide = (list) => {
    const listPos = list.map(T => {
        const n = calcTagPosition(T.id);
        n.tag = T;
        return n;
    });
    const outerRect = calcOuterRect(listPos);
    // find out item
    const oit = { xL: [], xU: [], zL: [], zU: [] };
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
    const center = { x: (outerRect.xU + outerRect.xL) / 2, y: (outerRect.zU + outerRect.zL) / 2 };
    // TODO
    return [];
};
// translate image (0,0) from LT to LB
const y2y = (imageY, y) => {
    return imageY - y;
};
const calcVectorLineScale = (va, vb) => {
    return MathEx.pythagoreanDistance(va.x, va.y) / MathEx.pythagoreanDistance(vb.x, vb.y);
};
const calcVec = (a, b) => {
    return {
        x: a.x - b.x,
        y: a.y - b.y,
    };
};
const calcK = (v) => {
    return v.y / v.x;
};
const calcPlaneInfo3 = (pla, img) => {
    const info = {};
    if (pla.length !== 3 || img.length !== 3) {
        throw Error("calcPlaneInfo3 (pla.length !== 3 || img.length !== 3)");
    }
    // TODO
    ;
    return info;
};
const calcImageCenterInPlane = (imageX, imageY, planeInfo) => {
    // TODO re-calc to update planeInfo P-P-pair
    return planeInfo;
};
function calc_map_position(tagInfo) {
    console.log("tagInfo:\n", JSON.stringify(tagInfo, undefined, 4));
    if (!checkTagInfoType(tagInfo) ||
        !checkAirplaneStateType(tagInfo.airplaneState) ||
        !checkTagType(tagInfo.tagInfo.center)) {
        return [false, 0, 0, 0];
    }
    if (!tagInfo.tagInfo.list.find(T => checkTagType(T))) {
        return [false, 0, 0, 0];
    }
    if (tagInfo.tagInfo.list.length < 2) {
        // type : calc by tag side size
        // TODO 1
    }
    else if (tagInfo.tagInfo.list.length === 2) {
        const l = tagInfo.tagInfo.list;
        if (l[0].id === l[1].id) {
            // type : calc by tag side size
            // TODO 1
        }
        else {
            // type : calc by 2 tag and it's direction
            const relation = calcTagRelation(l[0].id, l[1].id);
            console.log("relation: ", relation);
            // TODO 2
        }
    }
    else if (tagInfo.tagInfo.list.length >= 3) {
        const l = tagInfo.tagInfo.list;
        if (checkIsAllInSameLine(l)) {
            // type : calc by max distance 2 tag direction
            const relation = calcTagRelation(l[0].id, l[1].id);
            console.log("relation: ", relation);
            // TODO 2
        }
        else {
            // type : to find last triangle
            // TODO 3 findOuterSide
        }
    }
    return [true, 0, 1, 2];
}
//# sourceMappingURL=map_calc.js.map