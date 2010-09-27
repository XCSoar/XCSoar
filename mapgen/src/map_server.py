import os
import cherrypy
from waypoint_list import WaypointList
from map_daemon import MapJob
import pickle

'''
page 1 - area name, email address, input method (waypoints or manual)
page 2 - waypoint upload, additional border / coordinate entry
page 3 - terrain resolution
page 4 - status, download
'''

dir_temp = "../tmp"
dir_jobs = "../jobs"

class MapServer(object):
    def get_dir_job(self, job_id):
        dir_job = os.path.join(dir_jobs, job_id)
        if not os.path.exists(dir_job):
            os.mkdir(dir_job)
    
        self.lock_job(dir_job)    
        return dir_job
    
    def lock_job(self, dir_job):
        open(os.path.join(dir_job, "job.lock"), "w").close()
    
    def unlock_job(self, dir_job):
        lock_path = os.path.join(dir_job, "job.lock")
        if os.path.exists(lock_path):
            os.unlink(lock_path)
            
    def get_job_status(self, job_id):
        dir_job = os.path.join(dir_jobs, job_id)
        if not os.path.exists(dir_job):
            return None
        
        file_status = os.path.join(dir_job, "status")
        if not os.path.exists(file_status):
            return "Queued ..."
        
        f = open(file_status)
        ret = f.readline()
        f.close()
        
        return ret
    
    def header(self, subtitle = ""):
        return """
        <html>
        <head>
            <title>XCSoar - Map Server</title>
        </head>
        <body>
            <h1>XCSoar - Map Server</h1>
            <h2>""" + subtitle + """</h2>
        """

    def footer(self):
        return """
        </body>
        </html>
        """
        
    def surround(self, html, subtitle = ""):
        return self.header(subtitle) + html + self.footer()
    
    @cherrypy.expose
    def index(self, error = ""):
        html = error + """
        <form action="submit_start" method="post">
        Map name:<br/>
        <input type="text" name="name"/><br/>
        eMail address:<br/>
        <input type="text" name="mail1"/><br/>
        <input type="text" name="mail2"/><br/>
        Method:<br/>
        <input type="radio" name="method" value="waypoints" checked/>Waypoint file<br/>
        <input type="radio" name="method" value="coordinates"/>Coordinates<br/>
        <input type="submit">
        </form>
        """
        return self.surround(html, "General")
    
    @cherrypy.expose
    def submit_start(self, name, mail1, mail2, method = "waypoints"):
        name = name.strip()
        if name == "":
            return self.index("<h3>No map name given!</h3>")
        
        mail1 = mail1.strip()
        mail2 = mail2.strip()
        if mail1 != mail2:
            return self.index("<h3>eMail address don't match!</h3>")
        
        if mail1 == "" or mail2 == "":
            return self.index("<h3>No eMail address given!</h3>")
        
        cherrypy.session['name'] = name
        cherrypy.session['mail'] = mail1
        cherrypy.session['method'] = method
        cherrypy.session.save()
        self.get_dir_job(cherrypy.session.id)
        
        if method == "coordinates":
            return self.coordinates()
        else:
            return self.waypoints()
    
    def waypoints(self, error = ""):
        html = error + """
        <form action="submit_waypoints" method="post" enctype="multipart/form-data">
        Waypoint file:<br/>
        <input type="file" name="waypoint_file"/><br/>
        <input type="submit">
        </form>
        """
        return self.surround(html, "Waypoint file")
    
    @cherrypy.expose
    def submit_waypoints(self, waypoint_file):
        if not os.path.exists(waypoint_file.file.name):
            return self.waypoints("<h3>Waypoint file could not be read!</h3>")
            
        path = os.path.join(self.get_dir_job(cherrypy.session.id), 
                            waypoint_file.filename)
        f = open(path, "w")
        print f
        while True:
            data = waypoint_file.file.read(8192)
            if not data:
                break
            f.write(data)
        f.close()
        print path
            
        cherrypy.session['waypoint_file'] = path

        wplist = WaypointList()
        wplist.parse(open(path))
        bounds = wplist.get_bounds()
        print bounds
        cherrypy.session['bounds'] = bounds
        cherrypy.session.save()

        return self.terrain()

    def coordinates(self, error = ""):
        return self.surround("", "Coordinates")

    @cherrypy.expose
    def submit_manual(self, waypoint_file):
        return self.surround(cherrypy.session.get('mail'))
    
    def terrain(self, error = ""):
        html = error + """
        <form action="submit_terrain" method="post">
        Resolution:<br/>
        <input type="radio" name="resolution" value="3"/>3 arc-seconds<br/>
        <input type="radio" name="resolution" value="6"/>6 arc-seconds<br/>
        <input type="radio" name="resolution" value="9" checked/>9 arc-seconds<br/>
        <input type="radio" name="resolution" value="12"/>12 arc-seconds<br/>
        <input type="radio" name="resolution" value="15"/>15 arc-seconds<br/>
        <input type="radio" name="resolution" value="20"/>20 arc-seconds<br/>
        <input type="submit">
        </form>
        """
        return self.surround(html, "Terrain")

    @cherrypy.expose
    def submit_terrain(self, resolution):
        cherrypy.session['resolution'] = resolution
        cherrypy.session.save()
        
        dir_job = self.get_dir_job(cherrypy.session.id)
        file_job = os.path.join(dir_job, "job")
        
        job = MapJob()
        job.command = "generate"
        job.output_file = os.path.join(dir_job, "map.xcm")
        job.resolution = cherrypy.session.get("resolution")
        if cherrypy.session.get("method") == "coordinates":
            job.bounds = cherrypy.session.get("bounds")
        else:
            job.waypoint_file = cherrypy.session.get("waypoint_file")
            
        f = open(file_job, "wb")
        pickle.dump(job, f)
        f.close()
        
        self.unlock_job(dir_job)
        
        return self.status()
    
    @cherrypy.expose
    def status(self, job_id = None):
        if job_id == None:
            job_id = cherrypy.session.id
        
        status = self.get_job_status(job_id)
        if status == None:
            return self.surround("<h3>Job not found...</h3>", "Status")
        if status == "Done":
            return self.surround("<h3>Map ready for <a href=\"download?job_id=" +
                                 job_id + "\">Download</a>", "Status")
        
        return self.surround("<h3>" + status + "</h3><br/>" + 
                             "<a href=\"status?job_id=" + job_id + 
                             "\">Refresh</a>", "Status")
    
    @cherrypy.expose
    def download(self, job_id = None):
        dir_job = self.get_dir_job(job_id)
        file_map = os.path.abspath(os.path.join(dir_job, "map.xcm"))
        return cherrypy.lib.static.serve_download(file_map)
        
if __name__ == '__main__':
    cherrypy.config.update({'tools.sessions.on': True})
    cherrypy.config.update({'server.socket_port': 8037})
    cherrypy.quickstart(MapServer())