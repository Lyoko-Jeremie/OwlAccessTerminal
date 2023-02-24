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

    function maxIndex(...n: number[]): number;
}

declare module MathExOpenCV {

    /**
     * cv::getRotationMatrix2D
     * @param px
     * @param py
     * @param angle
     * @param scale
     * @return [2x3] AffineTransformMatrix
     */
    function getRotationMatrix2D(
        px: number, py: number,
        angle: number, scale: number,
    ): number[];

    /**
     * cv::getAffineTransform
     * @param p1xSrc
     * @param p1ySrc
     * @param p2xSrc
     * @param p2ySrc
     * @param p3xSrc
     * @param p3ySrc
     * @param p1xDst
     * @param p1yDst
     * @param p2xDst
     * @param p2yDst
     * @param p3xDst
     * @param p3yDst
     * @return [2x3] AffineTransformMatrix
     */
    function getAffineTransform(
        p1xSrc: number, p1ySrc: number,
        p2xSrc: number, p2ySrc: number,
        p3xSrc: number, p3ySrc: number,
        p1xDst: number, p1yDst: number,
        p2xDst: number, p2yDst: number,
        p3xDst: number, p3yDst: number,
    ): number[];

    /**
     * cv::invertAffineTransform
     * @param m [2x3] AffineTransformMatrix
     * @return [2x3] AffineTransformMatrix
     */
    function invertAffineTransform(m: number[]): number[];

    /**
     * cv::transform
     * @param pArray [2xn] Point2f list
     * @param m [2x3] AffineTransform
     * @return [2xn] Point2f list
     */
    function transform(pArray: number[], m: number[]): number[];

    /**
     * cv::convexHull
     * @param pArray [2xn] Point2f list
     * @return [n] pArray index list
     */
    function convexHull(pArray: number[]): number[];

    /**
     * cv::minEnclosingCircle
     * @param pArray [2xn] Point2f list
     * @return [center.x, center.y, radius]
     */
    function minEnclosingCircle(pArray: number[]): number[];

    /**
     * cv::boundingRect
     * @param pArray [2xn] Point2f list
     * @return [rect.x, rect.y, rect.width, rect.height]
     */
    function boundingRect(pArray: number[]): number[];

    /**
     * cv::minAreaRect
     * @param pArray [2xn] Point2f list
     * @return       [ rect.center.x, rect.center.y,
     *                 rect.size.width, rect.size.height,
     *                 rect.angle]
     */
    function minAreaRect(pArray: number[]): number[];

    /**
     * cv::minAreaRect_boxPoints
     * @param pArray [2xn] Point2f list
     * @return       [ p.at(0).x, p.at(0).y,
     *                 p.at(1).x, p.at(1).y,
     *                 p.at(2).x, p.at(2).y,
     *                 p.at(3).x, p.at(3).y,
     *                 rect.center.x, rect.center.y,
     *                 rect.size.width, rect.size.height,
     *                 rect.angle]
     */
    function minAreaRect_boxPoints(pArray: number[]): number[];

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

const calcVec = (a: Point2, b: Point2): Vec2 => {
    return {
        x: a.x - b.x,
        y: a.y - b.y,
    };
};

const calcK = (v: Vec2) => {
    return v.y / v.x;
};


const calcPlaneInfo = (pla: Point2[], img: Point2[], imgX: number, imgY: number):
    Pick<PlaneInfo, 'xDirect' | 'zDirect' | 'ImageP' | 'PlaneP'> => {
    const info = {} as Pick<PlaneInfo, 'xDirect' | 'zDirect' | 'ImageP' | 'PlaneP'>;

    if (pla.length !== 3 || img.length !== 3) {
        throw Error("calcPlaneInfo3 (pla.length !== 3 || img.length !== 3)");
    }

    const mInPla = MathExOpenCV.getAffineTransform(
        img[0].x, img[0].y,
        img[1].x, img[1].y,
        img[2].x, img[2].y,
        pla[0].x, pla[0].y,
        pla[1].x, pla[1].y,
        pla[2].x, pla[2].y,
    );
    const mInImg = MathExOpenCV.getAffineTransform(
        pla[0].x, pla[0].y,
        pla[1].x, pla[1].y,
        pla[2].x, pla[2].y,
        img[0].x, img[0].y,
        img[1].x, img[1].y,
        img[2].x, img[2].y,
    );

    const pImgInPla = MathExOpenCV.transform(
        [
            imgX / 2, imgY / 2,
            0, 0,
            imgX, 0,
            0, imgY,
            imgX, imgY,
        ],
        mInPla,
    );
    // TODO


    return info;
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
        //      calcPlaneInfo
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
            if (tagInfo.tagInfo.list.length === 3) {
                // type : calc use 3 point
                // TODO 3
                //      calcPlaneInfo
            }
            // type : to find last triangle
            // TODO 3 findOuterSide
            //      convexHull
        }
    }
    return [true, 0, 1, 2];
}
