# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant::Config.run do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "precise32"

  # The url from where the 'config.vm.box' box will be fetched if it
  # doesn't already exist on the user's system.
  config.vm.box_url = "http://files.vagrantup.com/precise32.box"

  # Use NFS shared folders to speed up the compilation on host systems
  # that support it.
  config.vm.share_folder("v-root", "/vagrant", ".", :nfs => (RUBY_PLATFORM =~ /linux/ or RUBY_PLATFORM =~ /darwin/))
  config.vm.network :hostonly, "192.168.42.2"

  # Enable provisioning with Puppet stand alone.  Puppet manifests
  # are contained in a directory path relative to this Vagrantfile.
  # You will need to create the manifests directory and a manifest in
  # the file precise32.pp in the manifests_path directory.
  config.vm.provision :puppet do |puppet|
    puppet.manifests_path = ".puppet/vagrant-manifests"
    puppet.manifest_file = "default.pp"
    puppet.module_path = ".puppet/modules"
  end
end
