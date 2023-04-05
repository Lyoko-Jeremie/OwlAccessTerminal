"use strict";
console.log("JS map_calc VERSION : ", 20230314);
const checkTagType = (t_) => {
    if (!t_) {
        return false;
    }
    const t = t_;
    return t
        && t.id !== undefined
        // && t.dec_marg !== undefined
        // && t.ham !== undefined
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
const AlgorithmMultiScale = 100;
// ============ map info ============
const MapX = 20;
const MapZ = 20;
// cm * AlgorithmMultiScale
const DistanceX = 50 * AlgorithmMultiScale;
// cm * AlgorithmMultiScale
const DistanceZ = 50 * AlgorithmMultiScale;
// cm * AlgorithmMultiScale
const SizeX = 10 * AlgorithmMultiScale;
// cm * AlgorithmMultiScale
const SizeZ = 10 * AlgorithmMultiScale;
const SizeXHalf = SizeX / 2;
const SizeZHalf = SizeZ / 2;
const OffsetX = 50 * AlgorithmMultiScale;
const OffsetZ = 50 * AlgorithmMultiScale;
const calcTagPosition = (t) => {
    const row = Math.floor(t / MapX);
    const col = t - row * MapX;
    return { x: col * DistanceX + OffsetX, y: row * DistanceZ + OffsetZ };
};
const calcTagCenterPosition = (t) => {
    return calcTagPosition(t);
};
const calcTagCornerPosition = (t, corner) => {
    const center = calcTagCenterPosition(t);
    switch (corner) {
        case 0:
            // LT
            center.x -= SizeXHalf;
            center.y += SizeZHalf;
            break;
        case 1:
            // RT
            center.x += SizeXHalf;
            center.y += SizeZHalf;
            break;
        case 2:
            // RB
            center.x += SizeXHalf;
            center.y -= SizeZHalf;
            break;
        case 3:
            // LB
            center.x -= SizeXHalf;
            center.y -= SizeZHalf;
            break;
    }
    return center;
};
const calcTagRelation = (t1, t2) => {
    const p1 = calcTagCenterPosition(t1);
    const p2 = calcTagCenterPosition(t2);
    const d = Math.atan2(p1.y - p2.y, p1.x - p2.x);
    return {
        // distance: Math.sqrt(Math.pow(Math.abs(p1.x - p2.x), 2) + Math.pow(Math.abs(p1.y - p2.y), 2)),
        distance: MathEx.distance(p1.x, p1.y, p2.x, p2.y),
        rad: d,
        deg: MathEx.radToDeg(d),
    };
};
const checkIsAllInSameLine = (list) => {
    const lp = list.map(T => calcTagCenterPosition(T.id));
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
        const n = calcTagCenterPosition(T.id);
        n.tag = T;
        return n;
    });
    const pArray = [];
    for (const p of listPos) {
        pArray.push(p.x);
        pArray.push(p.y);
    }
    if (pArray.length === 0) {
        return [];
    }
    const [cX, cY, r, ...pList] = MathExOpenCV.minEnclosingCircle(pArray);
    const pI = MathExOpenCV.circleBorderPoints(cX, cY, r, pList);
    if (pI.length > 3) {
        const pL = pI.map(I => list[I]);
        const pm = new Set(pL);
        while (true) {
            if (pm.size >= 3) {
                break;
            }
            const n = pL[MathEx.randomInt2(0, pL.length)];
            if (!pm.has(n)) {
                pm.add(n);
            }
        }
        return Array.from(pm.values());
    }
    else {
        return pI.map(I => list[I]);
    }
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
const calcPlaneInfo = (pla, img, imgX, imgY) => {
    const info = {};
    if (pla.length !== 3 || img.length !== 3) {
        throw Error("calcPlaneInfo3 (pla.length !== 3 || img.length !== 3)");
    }
    // 图像到平面的变换矩阵
    const mInPla = MathExOpenCV.getAffineTransform(img[0].x, img[0].y, img[1].x, img[1].y, img[2].x, img[2].y, pla[0].x, pla[0].y, pla[1].x, pla[1].y, pla[2].x, pla[2].y);
    // console.log("mInPla :\n", JSON.stringify(mInPla, undefined, 4));
    // 平面到图像的变换矩阵
    const mInImg = MathExOpenCV.getAffineTransform(pla[0].x, pla[0].y, pla[1].x, pla[1].y, pla[2].x, pla[2].y, img[0].x, img[0].y, img[1].x, img[1].y, img[2].x, img[2].y);
    // console.log("mInImg :\n", JSON.stringify(mInImg, undefined, 4));
    // 图片中心像素点xy
    const centerImgPoint = { x: imgX / 2, y: imgY / 2, };
    info.ImageP = centerImgPoint;
    const pImgInPla = MathExOpenCV.transform([
        // center
        centerImgPoint.x, centerImgPoint.y,
        // lt
        0, imgY,
        // rt
        imgX, imgY,
        // lb
        0, 0,
        // rb
        imgX, 0,
        // U
        centerImgPoint.x, centerImgPoint.y + 10000,
        // R
        centerImgPoint.x + 10000, centerImgPoint.y,
    ], mInPla);
    // console.log("pImgInPla [] :\n", JSON.stringify([
    //     // center
    //     centerImgPoint.x, centerImgPoint.y,
    //     // lt
    //     0, 0,
    //     // rt
    //     imgX, 0,
    //     // lb
    //     0, imgY,
    //     // rb
    //     imgX, imgY,
    // ], undefined, 4));
    // console.log("pImgInPla :\n", JSON.stringify(pImgInPla, undefined, 4));
    // 图像中心点对应的平面上的点的坐标
    const centerPlanPoint = { x: pImgInPla[0], y: pImgInPla[1] };
    info.PlaneP = {
        x: centerPlanPoint.x / AlgorithmMultiScale,
        y: centerPlanPoint.y / AlgorithmMultiScale,
    };
    // console.log("centerPlanPoint :\n", JSON.stringify(centerPlanPoint, undefined, 4));
    const offsetLen = 10000 * AlgorithmMultiScale;
    const pPlaInImg = MathExOpenCV.transform([
        // r
        centerPlanPoint.x + offsetLen, centerPlanPoint.y,
        // u
        centerPlanPoint.x, centerPlanPoint.y + offsetLen,
        // l
        centerPlanPoint.x - offsetLen, centerPlanPoint.y,
        // d
        centerPlanPoint.x, centerPlanPoint.y - offsetLen,
        // rt
        centerPlanPoint.x + offsetLen, centerPlanPoint.y + offsetLen,
        // U
        centerPlanPoint.x, centerPlanPoint.y - SizeZ * 1000,
        // R
        centerPlanPoint.x + SizeX * 1000, centerPlanPoint.y,
        // RU
        centerPlanPoint.x + SizeX * 1000, centerPlanPoint.y - SizeZ * 1000,
    ], mInImg);
    // console.log("pPlaInImg :\n", JSON.stringify(
    //     [
    //         // r
    //         centerPlanPoint.x + 100, centerPlanPoint.y,
    //         // u
    //         centerPlanPoint.x, centerPlanPoint.y + 100,
    //         // l
    //         centerPlanPoint.x - 100, centerPlanPoint.y,
    //         // d
    //         centerPlanPoint.x, centerPlanPoint.y - 100,
    //         // rt
    //         centerPlanPoint.x + 100, centerPlanPoint.y + 100,
    //     ], undefined, 4));
    // console.log("pPlaInImg :\n", JSON.stringify(pPlaInImg, undefined, 4));
    // 开始计算平面的右(x)向量相对于图像的旋转角度(从x轴正方向逆时针0~360)
    const imgR = {
        x: pPlaInImg[0 + 2 * 6],
        y: pPlaInImg[1 + 2 * 6],
    };
    // console.log("imgR :\n", JSON.stringify(imgR, undefined, 4));
    info.xDirectDeg = MathEx.atan2Deg(imgR.y, imgR.x);
    // 开始计算平面的上(z)向量相对于图像的旋转角度(从x轴正方向逆时针0~360)
    const imgU = {
        x: pPlaInImg[0 + 2 * 5],
        y: pPlaInImg[1 + 2 * 5],
    };
    // console.log("imgU :\n", JSON.stringify(imgU, undefined, 4));
    info.zDirectDeg = MathEx.atan2Deg(imgU.y, imgU.x);
    // 开始计算平面的上(xz)向量(45deg)相对于图像的旋转角度(从x轴正方向逆时针0~360)
    const imgRU = {
        x: pPlaInImg[0 + 2 * 7],
        y: pPlaInImg[1 + 2 * 7],
    };
    // console.log("imgU :\n", JSON.stringify(imgRU, undefined, 4));
    info.xzDirectDeg = MathEx.atan2Deg(imgRU.y, imgRU.x);
    // Scale
    // pixel per cm
    // ????
    const scalePlaInImg = {
        x: MathEx.pythagoreanDistance(pImgInPla[0 + 2 * 6], pImgInPla[1 + 2 * 6]) / 10000 / AlgorithmMultiScale,
        y: MathEx.pythagoreanDistance(pImgInPla[0 + 2 * 5], pImgInPla[1 + 2 * 5]) / 10000 / AlgorithmMultiScale,
    };
    // console.log("scalePlaInImg :\n", [pPlaInImg[0 + 2 * 6], pPlaInImg[1 + 2 * 6]]);
    // console.log("scalePlaInImg :\n", [pPlaInImg[0 + 2 * 5], pPlaInImg[1 + 2 * 5]]);
    // console.log("scalePlaInImg :\n", [SizeXHalf, 2, AlgorithmMultiScale]);
    // console.log("scalePlaInImg :\n", JSON.stringify(scalePlaInImg, undefined, 4));
    // console.log("scalePlaInImg :\n", MathEx.pythagoreanDistance(scalePlaInImg.x, scalePlaInImg.y));
    // console.log("scalePlaInImg :\n", MathEx.atan2Deg(scalePlaInImg.y, scalePlaInImg.x));
    // cm per pixel
    // ????
    const scaleImgInPla = {
        x: MathEx.pythagoreanDistance(pPlaInImg[0 + 2 * 6], pPlaInImg[1 + 2 * 6]) / (SizeX * 1000) * AlgorithmMultiScale,
        y: MathEx.pythagoreanDistance(pPlaInImg[0 + 2 * 5], pPlaInImg[1 + 2 * 5]) / (SizeZ * 1000) * AlgorithmMultiScale,
    };
    // console.log("scaleImgInPla :\n", [pImgInPla[0 + 2 * 6], pImgInPla[1 + 2 * 6]]);
    // console.log("scaleImgInPla :\n", JSON.stringify(scaleImgInPla, undefined, 4));
    // console.log("scaleImgInPla :\n", MathEx.pythagoreanDistance(scaleImgInPla.x, scaleImgInPla.y));
    // console.log("scaleImgInPla :\n", MathEx.atan2Deg(scaleImgInPla.y, scaleImgInPla.x));
    info.ScaleXY = scaleImgInPla;
    info.ScaleXZ = scalePlaInImg;
    return info;
};
function calc_map_position(tagInfo) {
    console.log("tagInfo:\n", JSON.stringify(tagInfo, undefined, 4));
    if (!checkTagInfoType(tagInfo) ||
        // !checkAirplaneStateType(tagInfo.airplaneState) ||
        !checkTagType(tagInfo.tagInfo.center)) {
        return { ok: false };
    }
    if (!tagInfo.tagInfo.list.find(T => checkTagType(T))) {
        return { ok: false };
    }
    const calcByCenterTag = () => {
        if (!checkTagType(tagInfo.tagInfo.center)) {
            return { ok: false };
        }
        const j = calcPlaneInfo([
            // LT
            calcTagCornerPosition(tagInfo.tagInfo.center.id, 0),
            // LB
            calcTagCornerPosition(tagInfo.tagInfo.center.id, 3),
            // RB
            calcTagCornerPosition(tagInfo.tagInfo.center.id, 2),
        ], [
            { x: tagInfo.tagInfo.center.cLTx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.center.cLTy) },
            { x: tagInfo.tagInfo.center.cLBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.center.cLBy) },
            { x: tagInfo.tagInfo.center.cRBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.center.cRBy) },
        ], tagInfo.imageX, tagInfo.imageY);
        console.log("j:\n", JSON.stringify(j, undefined, 4));
        return { ok: true, info: j };
    };
    return calcByCenterTag();
    // if (tagInfo.tagInfo.list.length < 2) {
    //     // type : calc by tag side size
    //     //      1
    //     //      calcPlaneInfo
    //     // console.log("calcTagCenterPosition :\n", JSON.stringify(calcTagCenterPosition(tagInfo.tagInfo.list[0].id), undefined, 4));
    //     const j = calcPlaneInfo(
    //         [
    //             // LT
    //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 0),
    //             // LB
    //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 3),
    //             // RB
    //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 2),
    //         ], [
    //             {x: tagInfo.tagInfo.list[0].cLTx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLTy)} as Point2,
    //             {x: tagInfo.tagInfo.list[0].cLBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLBy)} as Point2,
    //             {x: tagInfo.tagInfo.list[0].cRBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cRBy)} as Point2,
    //         ],
    //         tagInfo.imageX,
    //         tagInfo.imageY
    //     );
    //     console.log("j:\n", JSON.stringify(j, undefined, 4));
    //     return {ok: true, info: j};
    // } else if (tagInfo.tagInfo.list.length === 2) {
    //     const l = tagInfo.tagInfo.list;
    //     if (l[0].id === l[1].id) {
    //         // type : calc by tag side size
    //         const j = calcPlaneInfo(
    //             [
    //                 // LT
    //                 calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 0),
    //                 // LB
    //                 calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 3),
    //                 // RB
    //                 calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 2),
    //             ], [
    //                 {x: tagInfo.tagInfo.list[0].cLTx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLTy)} as Point2,
    //                 {x: tagInfo.tagInfo.list[0].cLBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLBy)} as Point2,
    //                 {x: tagInfo.tagInfo.list[0].cRBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cRBy)} as Point2,
    //             ],
    //             tagInfo.imageX,
    //             tagInfo.imageY
    //         );
    //         console.log("j:\n", JSON.stringify(j, undefined, 4));
    //         return {ok: true, info: j};
    //     } else {
    //         // type : calc by 2 tag, and it's direction
    //         // const relation = calcTagRelation(l[0].id, l[1].id);
    //         // console.log("relation: ", relation);
    //         //      way 2: calc by center tag
    //         const j = calcPlaneInfo(
    //             [
    //                 // LT
    //                 calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 0),
    //                 // LB
    //                 calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 3),
    //                 // RB
    //                 calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 2),
    //             ], [
    //                 {x: tagInfo.tagInfo.list[0].cLTx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLTy)} as Point2,
    //                 {x: tagInfo.tagInfo.list[0].cLBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLBy)} as Point2,
    //                 {x: tagInfo.tagInfo.list[0].cRBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cRBy)} as Point2,
    //             ],
    //             tagInfo.imageX,
    //             tagInfo.imageY
    //         );
    //         console.log("j:\n", JSON.stringify(j, undefined, 4));
    //         return {ok: true, info: j};
    //     }
    // } else if (tagInfo.tagInfo.list.length >= 3) {
    //     //      way 2: calc by center tag
    //     const j = calcPlaneInfo(
    //         [
    //             // LT
    //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 0),
    //             // LB
    //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 3),
    //             // RB
    //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 2),
    //         ], [
    //             {x: tagInfo.tagInfo.list[0].cLTx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLTy)} as Point2,
    //             {x: tagInfo.tagInfo.list[0].cLBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLBy)} as Point2,
    //             {x: tagInfo.tagInfo.list[0].cRBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cRBy)} as Point2,
    //         ],
    //         tagInfo.imageX,
    //         tagInfo.imageY
    //     );
    //     console.log("j:\n", JSON.stringify(j, undefined, 4));
    //     return {ok: true, info: j};
    //
    //
    //     // const l = tagInfo.tagInfo.list;
    //     // if (checkIsAllInSameLine(l)) {
    //     //     // type : calc by max distance 2 tag direction
    //     //     // const relation = calcTagRelation(l[0].id, l[1].id);
    //     //     // console.log("relation: ", relation);
    //     //     //      way 2: calc by center tag
    //     //     const j = calcPlaneInfo(
    //     //         [
    //     //             // LT
    //     //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 0),
    //     //             // LB
    //     //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 3),
    //     //             // RB
    //     //             calcTagCornerPosition(tagInfo.tagInfo.list[0].id, 2),
    //     //         ], [
    //     //             {x: tagInfo.tagInfo.list[0].cLTx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLTy)} as Point2,
    //     //             {x: tagInfo.tagInfo.list[0].cLBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cLBy)} as Point2,
    //     //             {x: tagInfo.tagInfo.list[0].cRBx, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cRBy)} as Point2,
    //     //         ],
    //     //         tagInfo.imageX,
    //     //         tagInfo.imageY
    //     //     );
    //     //     console.log("j:\n", JSON.stringify(j, undefined, 4));
    //     //     return {ok: true, info: j};
    //     // } else {
    //     //     if (tagInfo.tagInfo.list.length === 3) {
    //     //         // type : calc use 3 point
    //     //         //      3
    //     //         //      calcPlaneInfo
    //     //         const j = calcPlaneInfo(
    //     //             [
    //     //                 calcTagCenterPosition(tagInfo.tagInfo.list[0].id),
    //     //                 calcTagCenterPosition(tagInfo.tagInfo.list[1].id),
    //     //                 calcTagCenterPosition(tagInfo.tagInfo.list[2].id),
    //     //             ], [
    //     //                 {x: tagInfo.tagInfo.list[0].cY, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[0].cY)} as Point2,
    //     //                 {x: tagInfo.tagInfo.list[1].cY, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[1].cY)} as Point2,
    //     //                 {x: tagInfo.tagInfo.list[2].cY, y: y2y(tagInfo.imageY, tagInfo.tagInfo.list[2].cY)} as Point2,
    //     //             ],
    //     //             tagInfo.imageX,
    //     //             tagInfo.imageY
    //     //         );
    //     //         console.log("j:\n", JSON.stringify(j, undefined, 4));
    //     //         return {ok: true, info: j};
    //     //     }
    //     //     // type : to find last triangle
    //     //     //  3 findOuterSide
    //     //     //     x convexHull x
    //     //     const side3 = findOuterSide(tagInfo.tagInfo.list);
    //     //     if (side3.length !== 3) {
    //     //         // // type : to find last triangle
    //     //         // //  3 findOuterSide
    //     //         // //     x convexHull x
    //     //         // const side3 = findOuterSide(tagInfo.tagInfo.list);
    //     //         // if (side3.length !== 3) {
    //     //         //     return {ok: false};
    //     //         // }
    //     //         //
    //     //         // const j = calcPlaneInfo(
    //     //         //     [
    //     //         //         calcTagCenterPosition(side3[0].id),
    //     //         //         calcTagCenterPosition(side3[1].id),
    //     //         //         calcTagCenterPosition(side3[2].id),
    //     //         //     ], [
    //     //         //         {x: side3[0].cY, y: y2y(tagInfo.imageY, side3[0].cY)} as Point2,
    //     //         //         {x: side3[1].cY, y: y2y(tagInfo.imageY, side3[1].cY)} as Point2,
    //     //         //         {x: side3[2].cY, y: y2y(tagInfo.imageY, side3[2].cY)} as Point2,
    //     //         //     ],
    //     //         //     tagInfo.imageX,
    //     //         //     tagInfo.imageY
    //     //         // );
    //     //         // console.log("j:\n", JSON.stringify(j, undefined, 4));
    //     //         // return {ok: true, info: j};
    //     //     }
    //     // }
    // }
    // return {ok: false};
}
const testR = calc_map_position(JSON.parse(`{
    "imageX": 640,
    "imageY": 480,
    "tagInfo": {
        "center": {
            "id": 0,
            "dec_marg": 103.12965393066406,
            "ham": 0,
            "cX": 300.47344074876787,
            "cY": 159.54739680409008,
            "cLTx": 310.0415649414063,
            "cLTy": 191.97668457031247,
            "cRTx": 332.9658508300783,
            "cRTy": 149.8308715820313,
            "cRBx": 290.83453369140625,
            "cRBy": 126.87820434570318,
            "cLBx": 267.7102050781249,
            "cLBy": 169.34490966796872
        },
        "list": [
            {
                "id": 0,
                "dec_marg": 103.12965393066406,
                "ham": 0,
                "cX": 300.47344074876787,
                "cY": 159.54739680409008,
                "cLTx": 310.0415649414063,
                "cLTy": 191.97668457031247,
                "cRTx": 332.9658508300783,
                "cRTy": 149.8308715820313,
                "cRBx": 290.83453369140625,
                "cRBy": 126.87820434570318,
                "cLBx": 267.7102050781249,
                "cLBy": 169.34490966796872
            },
            {
                "id": 1,
                "dec_marg": 109.22723388671875,
                "ham": 0,
                "cX": 368.24851915605143,
                "cY": 31.903432995226193,
                "cLTx": 378.2727661132813,
                "cLTy": 65.40186309814452,
                "cRTx": 400.8331298828125,
                "cRTy": 22.363168716430668,
                "cRBx": 358.3687744140625,
                "cRBy": -1.1121082305908074,
                "cLBx": 336.38677978515625,
                "cLBy": 41.232051849365234
            },
            {
                "id": 20,
                "dec_marg": 103.37060546875,
                "ham": 0,
                "cX": 169.93853583374698,
                "cY": 89.31453153859457,
                "cLTx": 180.92633056640625,
                "cLTy": 122.7564697265625,
                "cRTx": 203.4572143554687,
                "cRTy": 79.63285064697264,
                "cRBx": 159.0496368408203,
                "cRBy": 56.173587799072266,
                "cLBx": 136.8728637695313,
                "cLBy": 98.86536407470705
        }
        ]
    },
    "airplaneState": {
        "timestamp": 0,
        "voltage": 0,
        "high": 0,
        "pitch": 0,
        "roll": 0,
        "yaw": 0,
        "vx": 0,
        "vy": 0,
        "vz": 0
    }
}`));
console.log("test R :", testR);
//# sourceMappingURL=map_calc.js.map