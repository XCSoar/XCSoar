require 'spec_helper'

describe "android" do
  default_version = '20.0.3'
  let(:facts) { {
    :operatingsystem => 'CentOS',
    :kernel => 'Linux',
    :osfamily => 'RedHat'
  } }

  context 'default' do
    it { should include_class('android::paths') }
    it { should include_class('android::sdk') }
    it { should include_class('android::platform_tools') }

    it { should contain_Wget__Fetch("download-androidsdk").with({ 
      :source => "http://dl.google.com/android/android-sdk_r#{default_version}-linux.tgz",
      :destination => "/usr/local/android/android-sdk_r#{default_version}-linux.tgz"}) 
    }
    it { should contain_exec('update-android-package-platform-tools')
        .with_command('/usr/local/android/android-sdk-linux/tools/android update sdk -u -t platform-tools  ') 
    }

    it { should contain_file('/usr/local/android').with( { :owner => 'root', :group => 'root' }) }
    it { should contain_exec('unpack-androidsdk').with( { :cwd => '/usr/local/android',:user => 'root' } ) }
  end

  context 'non-default version' do
    version = '2.0.0'
    let(:params) { {
      :version => version
    } }
    it { should contain_Wget__Fetch("download-androidsdk").with({ 
      :source => "http://dl.google.com/android/android-sdk_r#{version}-linux.tgz",
      :destination => "/usr/local/android/android-sdk_r#{version}-linux.tgz"}) 
    }

  end

  context 'with proxy' do
    let(:params) { {
      :proxy_host => 'myhost',
      :proxy_port => '1234'
    } }
    it { should contain_exec('update-android-package-platform-tools')
        .with_command('/usr/local/android/android-sdk-linux/tools/android update sdk -u -t platform-tools --proxy-host myhost --proxy-port 1234') }
    end

    context 'with installdir' do
      let(:params) { { :installdir => '/myinstalldir' } }
      it { should contain_file('/myinstalldir') }
      it { should contain_exec('unpack-androidsdk').with_cwd('/myinstalldir') }
    end

    context 'with different owner' do
      let(:params) { {
        :user => 'myuser',
        :group => 'mygroup'
      } }
      it { should contain_exec('unpack-androidsdk').with_user('myuser') }
      it { should contain_file('/usr/local/android').with( { :owner => 'myuser', :group => 'mygroup' } ) }
    end
    
    context 'Mac OS X' do
     let(:facts) { {
          :kernel => 'Darwin',
      } }

      it { should contain_Wget__Fetch("download-androidsdk").with({ 
        :source => "http://dl.google.com/android/android-sdk_r#{default_version}-macosx.zip",
        :destination => "/usr/local/android/android-sdk_r#{default_version}-macosx.zip"}) 
      }
      it { should contain_exec('update-android-package-platform-tools')
          .with_command('/usr/local/android/android-sdk-macosx/tools/android update sdk -u -t platform-tools  ') 
      }

      it { should contain_file('/usr/local/android').with( { :owner => 'root', :group => 'admin' }) }
      it { should contain_exec('unpack-androidsdk').with( { :cwd => '/usr/local/android',:user => 'root' } ) }
    end
end