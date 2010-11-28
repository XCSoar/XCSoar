import os
import cherrypy
from map_daemon import MapJob
import pickle
import hashlib
import random
import template
from genshi.filters import HTMLFormFiller

dir_jobs = "../jobs"

class MapServer(object):
    def generate_uuid(self):
        m = hashlib.sha1()
        m.update(str(random.random()))
        return m.hexdigest()

    def get_dir_job(self, uuid):
        return os.path.join(dir_jobs, uuid)

    def dir_job_exists(self, uuid):
        return os.path.exists(self.get_dir_job(uuid))

    def get_file_status(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "status")

    def get_file_job(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "job")

    def get_job_lock(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "job.lock")

    def get_download_lock(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "download.lock")

    def create_dir_job(self, uuid):
        dir_job = self.get_dir_job(uuid)
        if not os.path.exists(dir_job):
            os.mkdir(dir_job)
        return dir_job

    def lock_job(self, uuid):
        open(self.get_job_lock(uuid), "w").close()

    def unlock_job(self, uuid):
        job_lock = self.get_job_lock(uuid)
        if os.path.exists(job_lock):
            os.unlink(job_lock)

    def get_job_status(self, uuid):
        if not self.dir_job_exists(uuid):
            return None

        file_status = self.get_file_status(uuid)
        if not os.path.exists(file_status):
            return "Queued ..."

        f = open(file_status)
        ret = f.readline()
        f.close()

        return ret

    @cherrypy.expose
    @template.output('index.html')
    def index(self):
        return template.render()

    @cherrypy.expose
    @template.output('index.html')
    def generate(self, name, mail, waypoint_file):
        name = name.strip()
        if name == "":
            return template.render('index.html', error='No map name given!') | HTMLFormFiller(data=dict(name=name, mail=mail))

        mail = mail.strip()
        if mail == "":
            return template.render('index.html', error='No E-Mail adress given!') | HTMLFormFiller(data=dict(name=name, mail=mail))

        if not waypoint_file.file:
            return template.render('index.html', error='Waypoint file could not be read!') | HTMLFormFiller(data=dict(name=name, mail=mail))

        uuid = self.generate_uuid()

        dir_job = self.create_dir_job(uuid)
        self.lock_job(uuid)

        path = os.path.join(dir_job, "waypoints.dat")
        f = open(path, "w")
        while True:
            data = waypoint_file.file.read(8192)
            if not data:
                break
            f.write(data)
        f.close()

        file_job = self.get_file_job(uuid)

        job = MapJob()
        job.name = name
        job.mail = mail
        job.command = "generate"
        job.use_waypoint_file = True

        f = open(file_job, "wb")
        pickle.dump(job, f)
        f.close()

        self.unlock_job(uuid)

        return self.status(uuid)

    @cherrypy.expose
    @template.output('status.html')
    def status(self, uuid):
        status = self.get_job_status(uuid)
        if status == None:
            return template.render('error.html', error='Job not found!')
        if status == 'Done':
            return template.render('done.html', uuid=uuid, name='unknown map')
        return template.render(uuid=uuid, status=status, name='unknown map')

    @cherrypy.expose
    def download(self, uuid):
        if self.get_job_status(uuid) != "Done":
            return self.status(uuid)

        dir_job = self.get_dir_job(uuid)
        download_lock = self.get_download_lock(uuid)

        f = open(download_lock, "r")
        job = pickle.load(f)
        f.close()

        file_map = os.path.abspath(os.path.join(dir_job, "map.xcm"))
        return cherrypy.lib.static.serve_download(file_map, job.name + ".xcm")

if __name__ == '__main__':
    cherrypy.quickstart(MapServer(), '/', {
        '/static': {
            'tools.staticdir.on': True,
            'tools.staticdir.dir': 'static',
            'tools.staticdir.root': os.path.abspath(os.path.dirname(__file__)),
        }
    })
