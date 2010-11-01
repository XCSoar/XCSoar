import os
import cherrypy
from map_daemon import MapJob
import pickle
import hashlib
import random

dir_jobs = "../jobs"

class MapServer(object):
    def generate_uuid(self):
        m = hashlib.sha1()
        m.update(str(random.random()))
        return m.hexdigest()
    
    def get_dir_job(self, uuid):
        return os.path.join(dir_jobs, uuid)
    
    def create_dir_job(self, uuid):
        dir_job = self.get_dir_job(uuid)
        if not os.path.exists(dir_job):
            os.mkdir(dir_job)
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
    
    def header(self):
        return """<html>
<head>
    <title>XCSoar - Map Server</title>
    <link rel="stylesheet" type="text/css" href="/style" /> 
</head>
<body>
"""

    def footer(self):
        return """
</body>
</html>
"""
        
    def surround(self, html):
        return self.header() + html + self.footer()
    
    @cherrypy.expose
    def style(self):
        return """body { margin: 0px; font-family: "Lucida Grande", "Lucida Sans Unicode", "Lucida Sans", Helvetica, Arial, sans-serif; }
form p, div { margin-bottom: 20px; padding-right: 20px; font-family: Georgia, serif; font-size: 22px; line-height: 40px; font-weight: normal; }
form div.box { width: 580px; padding: 10px; margin-bottom: 4px; border: 1px solid #9f9c99; border-radius: 8px; font-family: "Lucida Grande", "Lucida Sans Unicode", "Lucida Sans", Helvetica, Arial, sans-serif;  font-size: 20px; }
form input.text { width: 100%; border: 0px; font-family: "Lucida Grande", "Lucida Sans Unicode", "Lucida Sans", Helvetica, Arial, sans-serif;  font-size: 20px; }
form input.button { width: 200px; cursor: pointer; padding: 8px 20px; margin-left: 205px; font-size: 18px; text-transform: uppercase; border: 0; border-radius: 8px; cursor: pointer; }
form input.button:hover, form input.button:focus { background: #1B8D29; color: #fff; }
"""

    @cherrypy.expose
    def index(self):
        html = """<form action="/generate/" method="post" enctype="multipart/form-data">
    <div>
        Map name:<br/>
        <div class="box">
            <input type="text" name="name" class="text" />
        </div>
    </div>
    <div>
        eMail:<br/>
        <div class="box">
            <input type="text" name="mail1" class="text" /><br/>
        </div>
        <div class="box">
            <input type="text" name="mail2" class="text" />
        </div>
    </div>
    <div>
        Waypoint file:<br/>
        <div class="box">
            <input type="file" name="waypoint_file" />
        </div>
    </div>
    <div>
        <input type="submit" value="Generate" class="button" id="submit">
    </div>
</form>"""
        return self.surround(html)
    
    def error(self, error):
        return self.surround(error)
    
    @cherrypy.expose
    def generate(self, name, mail1, mail2, waypoint_file):
        name = name.strip()
        if name == "":
            return self.error("No map name given!")
        
        mail1 = mail1.strip()
        mail2 = mail2.strip()
        if mail1 != mail2:
            return self.error("eMail addresses don't match!")
        
        if mail1 == "" or mail2 == "":
            return self.error("No eMail address given!")
        
        if not os.path.exists(waypoint_file.file.name):
            return self.error("Waypoint file could not be read!")
            
        uuid = self.generate_uuid()
        dir_job = self.create_dir_job(uuid)
        self.lock_job(dir_job)        
        
        path = os.path.join(dir_job, "waypoints.dat")
        f = open(path, "w")
        while True:
            data = waypoint_file.file.read(8192)
            if not data:
                break
            f.write(data)
        f.close()

        file_job = os.path.join(dir_job, "job")
        
        job = MapJob()
        job.command = "generate"
        job.use_waypoint_file = True
           
        f = open(file_job, "wb")
        pickle.dump(job, f)
        f.close()
       
        self.unlock_job(dir_job)
       
        return self.status(uuid)
 
    @cherrypy.expose
    def status(self, uuid):
        status = self.get_job_status(uuid)
        if status == None:
            return self.error("Job not found!")
        
        if status == "Done":
            return self.surround("Map ready for <a href=\"/download?uuid=" +
                                 uuid + "\">Download</a>")
        
        return self.surround(status + "<br/>" + 
                             "<a href=\"/status?uuid=" + uuid + 
                             "\">Refresh</a>")
    
    @cherrypy.expose
    def download(self, uuid):
        dir_job = self.get_dir_job(uuid)
        file_map = os.path.abspath(os.path.join(dir_job, "map.xcm"))
        return cherrypy.lib.static.serve_download(file_map)
        
if __name__ == '__main__':
    cherrypy.config.update({'server.socket_port': 8037})
    cherrypy.quickstart(MapServer())