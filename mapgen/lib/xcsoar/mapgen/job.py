import hashlib
import random
import pickle
import time
import os
import shutil

class JobDescription:
    name = None
    mail = None
    waypoint_file = None
    waypoint_details_file = None
    airspace_file = None
    use_topology = True
    use_terrain = True
    bounds = None
    resolution = 9.0

class Job:
    description = JobDescription()

    def __init__(self, dir_jobs, load=False):
        if load:
            self.dir  = dir_jobs
            self.uuid = os.path.basename(self.dir).split('.')[0]
            self.description = pickle.load(file(self.__job_file()))
        else:
            self.uuid = self.__generate_uuid()
            self.dir = os.path.join(dir_jobs, self.uuid + '.locked')
            if not os.path.exists(self.dir):
                os.mkdir(self.dir)

    def enqueue(self):
        f = open(self.__job_file(), 'wb')
        pickle.dump(self.description, f)
        f.close()
        self.__move('.queued')

    def file_path(self, name):
        return os.path.join(self.dir, name)

    def map_file(self):
        return self.file_path('map.xcm')

    def __status_file(self):
        return self.file_path('status')

    def __job_file(self):
        return self.file_path('job')

    def error(self):
        self.__move('.error')
        os.unlink(self.__status_file())

    def done(self):
        self.__move()
        os.unlink(self.__status_file())

    def update_status(self, status):
        f = open(self.__status_file(), 'w')
        f.write(status)
        f.close()

    def delete(self):
        shutil.rmtree(self.dir)

    def status(self):
        path = self.__status_file()
        if os.path.exists(path):
            return file(path).read()
        name = os.path.basename(self.dir)
        i = name.find('.')
        if i == -1:
            return 'Done'
        return name[i+1:].capitalize()

    def __move(self, postfix = ''):
        old = self.dir
        self.dir = os.path.join(os.path.dirname(old), self.uuid + postfix)
        os.rename(old, self.dir)

    def __generate_uuid(self):
        m = hashlib.sha1()
        m.update(str(random.random()))
        return m.hexdigest()

    @staticmethod
    def delete_expired(dir, max_age):
        if (time.time() - os.path.getctime(dir) > max_age):
            shutil.rmtree(dir)

    @staticmethod
    def load(dir):
        return Job(dir, True)

    @staticmethod
    def find(dir_jobs, uuid):
        base = os.path.join(dir_jobs, uuid)
        for suffix in ['', '.locked', '.queued', '.working', '.error']:
            if os.path.exists(base + suffix):
              return Job(base + suffix, True)
        return None

    @staticmethod
    def get_next(dir_jobs):
        for file in os.listdir(dir_jobs):
            dir = os.path.join(dir_jobs, file)

            # Only directories can be jobs
            if not os.path.isdir(dir):
                continue

            # Check if the job is locked by the creator
            # or if there is already a worker working on it
            if dir.endswith('.locked') or dir.endswith('.working'):
                Job.delete_expired(dir, 60*60)
                continue

            # Find an enqueued job
            if dir.endswith('.queued'):
                job = Job(dir, True)
                job.__move('.working')
                return job

            # Delete if download is expired
            Job.delete_expired(dir, 24*7*60*60)

        return None
