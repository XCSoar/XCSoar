var BoxWithStartCallback = OpenLayers.Class(OpenLayers.Handler.Box, {
    initialize: function(control, callbacks, options) {
        OpenLayers.Handler.Box.prototype.initialize.apply(this, arguments);
    },
    startBox: function(xy) {
        OpenLayers.Handler.Box.prototype.startBox.apply(this, arguments);
        this.callback('start', []);
    },
    CLASS_NAME: 'BoxWithStartCallback'
});

(function() {
    var projWgs84 = new OpenLayers.Projection('EPSG:4326');
    var projSphericalMercator = new OpenLayers.Projection('EPSG:900913');

    map = new OpenLayers.Map('map');
    var osm = new OpenLayers.Layer.OSM(null, null);
    var vector = new OpenLayers.Layer.Vector("Vector Layer");
    map.addLayers([osm, vector]);
    map.setCenter(new OpenLayers.LonLat(0, 20.0).transform(projWgs84, projSphericalMercator), 2);

    var control = new OpenLayers.Control();
    OpenLayers.Util.extend(control, {
         draw: function () {
             this.box = new BoxWithStartCallback(control,
                                                 { start: this.startBoundsSelection,
                                                   done: this.boundsSelected },
                                                 { keyMask: OpenLayers.Handler.MOD_CTRL });
             this.box.activate();
         },
         startBoundsSelection: function() {
             vector.removeAllFeatures();
         },
         boundsSelected: function(bounds) {
             var bottomleft = map.getLonLatFromPixel(new OpenLayers.Pixel(bounds.left, bounds.bottom));
             var topright = map.getLonLatFromPixel(new OpenLayers.Pixel(bounds.right, bounds.top));

             var rect = new OpenLayers.Geometry.LinearRing();
             rect.addPoint(new OpenLayers.Geometry.Point(bottomleft.lon, bottomleft.lat));
             rect.addPoint(new OpenLayers.Geometry.Point(bottomleft.lon, topright.lat));
             rect.addPoint(new OpenLayers.Geometry.Point(topright.lon,   topright.lat));
             rect.addPoint(new OpenLayers.Geometry.Point(topright.lon,   bottomleft.lat));

             this.bounds = new OpenLayers.Feature.Vector(rect);
             vector.addFeatures([this.bounds]);

	     bottomleft = bottomleft.transform(projSphericalMercator, projWgs84);
	     topright = topright.transform(projSphericalMercator, projWgs84);

	     var bounds = document.forms[0].elements;
	     bounds.latmin.value = bottomleft.lat;
	     bounds.latmax.value = topright.lat;
	     bounds.lonmin.value = bottomleft.lon;
	     bounds.lonmax.value = topright.lon;
         }
    });

    map.addControl(control);
})();
