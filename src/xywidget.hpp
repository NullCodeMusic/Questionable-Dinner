#pragma once

#include <rack.hpp>
#include <vector>

using namespace rack;

enum XYShapes {
    SHP_CIRCLE,
    SHP_SQUARE,
    SHP_TRIUP,
    SHP_TRIDOWN,
    SHP_DIAMOND,
    SHP_PENTAGON,
    SHP_HEXAGON
};

enum Bounds {
    B_TOP,
    B_BOTTOM,
    B_LEFT,
    B_RIGHT,
    BTYPE_RECT = 0,
    BTYPE_RADIUS
};

static const NVGcolor defaultScreenCol = nvgRGBAf(0.83529411764,0.83137254902,0.83137254902,1);

inline void ShapeFromID(NVGcontext *ctx, int id, float x, float y, float radius){
    switch (id)
    {
    case SHP_CIRCLE:
        nvgCircle(ctx,x,y,radius);
        break;
    case SHP_SQUARE:
        nvgRect(ctx,x-radius,y-radius,2*radius,2*radius);
    default:
        break;
    }
}

inline void lineSegment(NVGcontext *ctx, float x1, float y1, float x2, float y2){
    nvgBeginPath(ctx);
    nvgMoveTo(ctx,x1,y1);
    nvgLineTo(ctx,x2,y2);
    nvgStroke(ctx);
}

struct XYPoint {
    int shape;
    std::string label;
    Vec pos = Vec(0);
    NVGcolor color = defaultScreenCol;

    XYPoint(std::string label = " ", int shape = SHP_CIRCLE, NVGcolor color = defaultScreenCol){
        this->label = label;
        this->shape = shape;
        this->color = color;
    }
    XYPoint(Vec pos, std::string label = " ", int shape = SHP_CIRCLE, NVGcolor color = defaultScreenCol){
        this->label = label;
        this->shape = shape;
        this->pos = pos;
        this->color = color;
    }
    const char* getLabel(){
        return label.c_str();
    }
};

struct XYScreen : OpaqueWidget{
    float bounds[4] = {-0.9f,0.9f,-0.9f,0.9f};
    int gridSize = 10;
    float pointSizeRaw = 10;  
    bool gridsnap = false;
    NVGcolor bgCol = nvgRGBf(0.09803921568f,0.07843137254f,0.07450980392f);
    NVGcolor gCol = nvgRGBAf(1,1,1,0.4);
    private: 
    std::vector<XYPoint*> points;
    bool active = false;
    int selectedPoint = -1;
    Vec pointSizeRel;
    Vec cMousePosRel;

    public:
    void addPoint(XYPoint *point){
        points.push_back(point);
    }

    XYPoint* getPoint(int idx){
        return points.at(idx);
    }

    void onHover(const HoverEvent &e) override{
        e.consume(this);
        if(active){return;}

        pointSizeRel = Vec(pointSizeRaw)/box.size;
        Vec mousePosRel = relative(e.pos);
        selectedPoint = -1;
        int size = points.size();
        for(int i = size-1;i >= 0;i--){
            XYPoint* point = points.at(i);
            if(abs(mousePosRel.x-point->pos.x)<pointSizeRel.x&&abs(mousePosRel.y-point->pos.y)<pointSizeRel.y){
                selectedPoint = i;
                break;
            }
        }
    }

    inline void enforceBounds(){
        int size = points.size();
        for(int i = size-1;i >= 0;i--){
            XYPoint* point = points.at(i);
            point->pos.x = clamp(point->pos.x,bounds[B_LEFT],bounds[B_RIGHT]);
            point->pos.y = clamp(point->pos.y,bounds[B_TOP],bounds[B_BOTTOM]);
        }
    }

    void onDragStart(const DragStartEvent &e) override{
        e.consume(this);
        if(selectedPoint == -1){return;}

        XYPoint* point = points.at(selectedPoint);
        cMousePosRel = point->pos;
    }

    void onDragMove(const DragMoveEvent &e) override{
        e.consume(this);
        if(selectedPoint == -1){return;}

        XYPoint* point = points.at(selectedPoint);
        //cMousePosRel = (APP->scene->getMousePos()-getAbsoluteOffset(box.size/2))/box.size/getAbsoluteZoom()*2;
        cMousePosRel += e.mouseDelta/box.size/getAbsoluteZoom()*2;
        Vec moveToPos = Vec(clamp(cMousePosRel.x,bounds[B_LEFT],bounds[B_RIGHT]),clamp(cMousePosRel.y,bounds[B_TOP],bounds[B_BOTTOM]));

        point->pos = moveToPos;
    }

    void onDragDrop(const DragDropEvent &e) override{
        e.consume(this);
        active = false;
        selectedPoint = -1;
    }

    void draw(const DrawArgs& args) override {

        

    }
    
    void drawLayer(const DrawArgs& args, int layer) override {
        std::string fontPath = asset::plugin(pluginInstance, "res/fonts/RX100-Regular.otf");
        std::shared_ptr<Font> font = APP->window->loadFont(fontPath);

        if (layer==1)
        {
            nvgShapeAntiAlias(args.vg,1);
            nvgSave(args.vg);
            Vec tl = raw(Vec(bounds[B_LEFT],bounds[B_TOP]));
            Vec br = raw(Vec(bounds[B_RIGHT],bounds[B_BOTTOM]));

            nvgStrokeColor(args.vg, gCol);
            nvgBeginPath(args.vg);
            nvgRect(args.vg, tl.x, tl.y, br.x-tl.x, br.y-tl.y);
            nvgStroke(args.vg);

            float gridStep = 1.f/gridSize;
            nvgAlpha(args.vg,0.2);
            float boxX = box.size.x;
            float boxY = box.size.y;
            float boxX2 = boxX/2;
            float boxY2 = boxY/2;
            for(int i = 1;i < gridSize;i++){
                lineSegment(args.vg,i*gridStep*boxX,0,i*gridStep*boxX,boxY);
                lineSegment(args.vg,0,i*gridStep*boxY,boxX,i*gridStep*boxY);
                lineSegment(args.vg,i*gridStep*boxX,bounds[B_BOTTOM]*boxY2+boxY2,i*gridStep*boxX,bounds[B_TOP]*boxY2+boxY2);
                lineSegment(args.vg,bounds[B_LEFT]*boxX2+boxX2,i*gridStep*boxY,bounds[B_RIGHT]*boxX2+boxX2,i*gridStep*boxY);
            }

            nvgRestore(args.vg);

            int size = points.size();
            for(int i = 0;i < size;i++){
                XYPoint* point = points.at(i);
                if(i!=selectedPoint){
                    Vec rawPos = raw(point->pos);

                    nvgBeginPath(args.vg);
                    nvgFontFaceId(args.vg, font->handle);
                    nvgTextAlign(args.vg,NVG_ALIGN_MIDDLE|NVG_ALIGN_CENTER);
                    nvgFontSize(args.vg, 12.0f);
                    nvgFillColor(args.vg,point->color);
                    nvgStrokeWidth(args.vg,0.f);
                    nvgText(args.vg,rawPos.x,rawPos.y-0.4,point->getLabel(),NULL);

                }else{
                    Vec rawPos = raw(point->pos);
                    NVGcolor strokecol = point->color;
                    strokecol.a = 0.4;
                    nvgStrokeWidth(args.vg,2.f);
                    nvgStrokeColor(args.vg,strokecol);
                    nvgFillColor(args.vg,point->color);
                    nvgBeginPath(args.vg);
                    ShapeFromID(args.vg,point->shape,rawPos.x,rawPos.y,pointSizeRaw/2);
                    nvgFill(args.vg);
                    nvgStroke(args.vg);

                    nvgBeginPath(args.vg);
                    nvgFontFaceId(args.vg, font->handle);
                    nvgTextAlign(args.vg,NVG_ALIGN_MIDDLE|NVG_ALIGN_CENTER);
                    nvgFontSize(args.vg, 12.0f);
                    nvgFillColor(args.vg,bgCol);
                    nvgText(args.vg,rawPos.x,rawPos.y-0.4,point->getLabel(),NULL);
                }
            }
        }
	}

    void setBounds(float width,float height){
        int size = points.size();
        for(int i = size-1;i >= 0;i--){
            XYPoint* point = points.at(i);
            point->pos.x /= bounds[B_RIGHT];
            point->pos.y /= bounds[B_BOTTOM];
        }

        bounds[B_TOP] = -height;
        bounds[B_BOTTOM] = height;
        bounds[B_LEFT] = -width;
        bounds[B_RIGHT] = width;

        for(int i = size-1;i >= 0;i--){
            XYPoint* point = points.at(i);
            point->pos.x *= width;
            point->pos.y *= height;
        }
        
    }

    void setBoundsClamp(float width,float height){
        bounds[B_TOP] = -height;
        bounds[B_BOTTOM] = height;
        bounds[B_LEFT] = -width;
        bounds[B_RIGHT] = width;
        enforceBounds();
    }

    Vec relative(Vec rawPos){
        return rawPos/box.size*2-1;
    }

    Vec raw(Vec relativePos){
        return (relativePos+1)*box.size/2;
    }
};

struct PointList {
    std::vector<XYPoint*> points;
    std::vector<ParamQuantity*> xParams;
    std::vector<ParamQuantity*> yParams;
    XYScreen* screen;
    PointList(std::vector<XYPoint*> points, std::vector<ParamQuantity*> xParams, std::vector<ParamQuantity*> yParams){
        this->points = points;
        this->xParams = xParams;
        this->yParams = yParams;

    }
    void readFromParams(){
        int size = xParams.size();
        for(int i = 0; i<size; i++){
            if(i<points.size()){
                points.at(i)->pos.x = xParams.at(i)->getValue();
            }
        }
        size = yParams.size();
        for(int i = 0; i<size; i++){
            if(i<points.size()){
                points.at(i)->pos.y = -yParams.at(i)->getValue();
            }
        }
    }
    void writeToParams(){
        int size = points.size();
        for(int i = 0; i<size; i++){
            if(i<xParams.size()){
                xParams.at(i)->setValue(points.at(i)->pos.x);
            }
            if(i<yParams.size()){
                yParams.at(i)->setValue(-points.at(i)->pos.y);
            }
        }
    }
    void addPointsToScreen(XYScreen* screen){
        this->screen = screen;
        for(XYPoint* p : points){
            screen->addPoint(p);
        }
        screen->enforceBounds();
    }
};