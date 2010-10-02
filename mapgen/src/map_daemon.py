import os
import time

from map_generator import MapGenerator
import pickle

class MapJob:
    use_waypoint_file = False
    use_waypoint_details_file = False
    use_airspace_file = False
    use_topology = True
    use_terrain = True
    bounds = None
    resolution = 9.0
    
    def __init__(self, command = "generate"):
        self.command = command

class MapDaemon:
    def __init__(self, dir_jobs = "../jobs/"):
        self.__dir_jobs = os.path.abspath(dir_jobs)
        self.__run = False
    
    def __lock_download(self, dir_job):
        open(os.path.join(dir_job, "download.lock"), "w").close()
    
    def __delete_job(self, file_job, complete = True):
        dir_job = os.path.dirname(file_job)
        if complete:
            for file in os.listdir(dir_job):
                os.unlink(os.path.join(dir_job, file))
            os.rmdir(dir_job)
        else:
            if os.path.exists(file_job):
                os.unlink(file_job)
            
        print "Job deleted (" + file_job + ")"
        
    def __read_job(self, file_job):
        # Check if the job file exists
        if not os.path.exists(file_job):
            return None
        
        # Open the job file for reading
        f = open(file_job, "r")
        # Unpickle the MapJob instance
        job = pickle.load(f)
        # Close the job file again
        f.close()
        
        # Save the job file path and folder path in the MapJob instance
        job.file_job = file_job
        job.dir_job = os.path.dirname(file_job)
        
        return job
    
    def __update_job_status(self, dir_job, status):
        if not os.path.exists(dir_job):
            return
        
        f = open(os.path.join(dir_job, "status"), "w")
        f.write(status)
        f.close()
    
    def __check_job_lock_expired(self, file_lock):
        return (time.time() - os.path.getctime(file_lock) > 60 * 60 * 2)
    
    def __check_download_lock_expired(self, file_lock):
        return (time.time() - os.path.getctime(file_lock) > 60 * 60 * 24 * 7)
    
    def __check_jobs(self):
        # Iterate through files/folders in our jobs folder
        for file in os.listdir(self.__dir_jobs):
            dir_job = os.path.join(self.__dir_jobs, file)
            
            # Skip any non-folder entries
            if not os.path.isdir(dir_job):
                continue

            file_job = os.path.join(dir_job, "job")

            # Check if the job is locked by the creator
            if os.path.exists(file_job + ".lock"):
                # Check if the lock is expired
                if self.__check_job_lock_expired(file_job + ".lock"):
                    # If expired -> Delete the outdated job
                    print "---------------------"
                    print "Job lock expired (" + file_job + ".lock)"
                    self.__delete_job(file_job)
                # If you exists -> Continue with next job
                continue
            
            # If the job isn't locked -> Check if the job file exists
            if not os.path.exists(file_job):
                # If no job file exists -> Check if the download lock exists
                file_download = os.path.join(dir_job, "download.lock")
                if os.path.exists(file_download):
                    # Check if download lock is expired
                    if self.__check_download_lock_expired(file_download):
                        # If expired -> Delete the job
                        print "---------------------"
                        print "Download lock expired (" + file_download + ")"
                        self.__delete_job(file_job)
                # If no job file exists -> Continue with next job
                continue

            # We've found a job! -> Try to read the MapJob instance from the job file
            print "---------------------"
            print "Job found (" + file_job + ")"
            job = self.__read_job(file_job)
            if job != None:
                return job
            
            # If job file could not be read -> Delete the job
            self.__delete_job(file_job)
        
        # Not jobs found
        return None
    
    def __execute_job(self, job):
        # Check for "generate" command
        if job.command == "generate":
            print "Command \"generate\" found. Generating map file."
            
            # Check if there is anything defining the boundaries
            if job.use_waypoint_file == False and job.bounds == None:
                print "No waypoint file or bounds set. Aborting."
                # If not -> Delete the job
                self.__delete_job(job.file_job)
                return

            # Create a MapGenerator instance for creating the map file
            m = MapGenerator()
            
            # Check if we should use a waypoint file
            if job.use_waypoint_file:
                self.__update_job_status(job.dir_job, 
                                         "Adding waypoint file...")
                # Add the waypoint file to the MapGenerator
                m.AddWaypointFile(os.path.join(job.dir_job, "waypoints.dat"))
                
            # If the job has fixed bounds defined 
            if job.bounds != None:
                # ... use them
                m.SetBounds(job.bounds)
            else:
                # If not -> Calculate the bounds of the waypoint file
                m.SetBoundsByWaypointFile(os.path.join(job.dir_job, 
                                                       "waypoints.dat"))
            
            # Check if we should create topology files
            if job.use_topology:
                self.__update_job_status(job.dir_job, 
                                         "Creating topology files...")
                m.AddTopology()

            # Check if we should create terrain files
            if job.use_terrain:
                self.__update_job_status(job.dir_job, 
                                         "Creating terrain files...")
                m.AddTerrain(job.resolution)
            
            # Check if we should use a waypoint details file
            if job.use_waypoint_details_file:
                self.__update_job_status(job.dir_job, 
                                         "Adding waypoint details file...")
                # Add the waypoint details file to the MapGenerator
                m.AddWaypointDetailsFile(os.path.join(job.dir_job, "details.txt"))
                
            # Check if we should use a airspace file
            if job.use_airspace_file:
                self.__update_job_status(job.dir_job, 
                                         "Adding airspace file...")
                # Add the airspace file to the MapGenerator
                m.AddAirspaceFile(os.path.join(job.dir_job, "sua.txt"))
                
            self.__update_job_status(job.dir_job, 
                                     "Creating map file...")
            # Create the map file
            m.Create(os.path.join(job.dir_job, "map.xcm"))
            # Clean up the temporary files
            m.Cleanup()
            self.__update_job_status(job.dir_job,  
                                     "Done")
            print "Map ready for use (" + os.path.join(job.dir_job, "map.xcm") + ")"
            
            # Activate the download lock
            self.__lock_download(job.dir_job)
            # Delete job file
            self.__delete_job(job.file_job, False)
            return
        
        # Check for "stop" command
        if job.command == "stop":
            print "Command \"stop\" found. Stopping daemon."
            # Delete the "stop" job
            self.__delete_job(job.file_job)
            # ... and stop the daemon 
            self.__run = False
            return
        
        print "No known command given inside MapJob object"
        self.__delete_job(job.file_job)
                
    def Run(self):
        print "Starting MapDaemon ..."
        self.__run = True
        
        print "Monitoring " + self.__dir_jobs + " for new jobs ..."
        while self.__run:
            # Check if there are new jobs
            job = self.__check_jobs()
            if job != None:
                # If we found a job -> execute it
                self.__execute_job(job)
            else:
                # Otherwise sleep for a little while
                time.sleep(0.5)
    
if __name__ == '__main__':    
    md = MapDaemon()
    md.Run()
    