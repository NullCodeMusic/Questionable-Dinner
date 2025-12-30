#include "plugin.hpp"
#include "palettes.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelASawB);
	p->addModel(modelPilfer);
	p->addModel(modelSurgeon);
	p->addModel(modelElasticTwins);
	p->addModel(modelLoam);
	p->addModel(modelFlick);
	p->addModel(modelFlickX);
	p->addModel(modelYare);
	p->addModel(modelGrit);
	p->addModel(modelMoxie);
	p->addModel(modelNotable);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}

QTintPanel* createTintPanel(std::string path, std::array<unsigned int, 4> colors){
	QTintPanel* panel = new QTintPanel;
	panel->setBackground(Svg::load(asset::plugin(pluginInstance,path)));
	panel->panel = Svg::load(asset::plugin(pluginInstance,path));
	panel->setColors(colors);
	return panel;
}