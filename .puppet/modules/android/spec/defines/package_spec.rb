require 'spec_helper'

describe 'android::package' do

  context 'linux' do
    let(:facts) { {
      :operatingsystem => 'CentOS',
      :kernel => 'Linux',
      :osfamily => 'RedHat'
    } }
    let(:title) { 'android-15' }
    let(:params) { { :type => 'platform' } }
    creates = '/usr/local/android/android-sdk-linux/platforms/android-15'
    it { should contain_exec('update-android-package-android-15').with({
      :command => '/usr/local/android/android-sdk-linux/tools/android update sdk -u -t android-15  ',
      :creates => creates,
    }) }
  end
  
  context 'bad package type' do
    let(:facts) { {
      :operatingsystem => 'CentOS',
      :kernel => 'Linux',
      :osfamily => 'RedHat'
    } }
    let(:title) { 'bad' }
    let(:params) { { :type => 'bad' } }
    
    it do
      expect {
        should contain_exec('update-android-package-bad') 
      }.to raise_error(Puppet::Error, /Unsupported package type: bad/) 
    end
  end
  
  context 'Mac OS X' do
    let(:facts) { {
      :kernel => 'Darwin',
    } }
    let(:title) { 'android-15' }
    let(:params) { { :type => 'platform' } }
    creates = '/usr/local/android/android-sdk-macosx/platforms/android-15'
    it { should contain_exec('update-android-package-android-15').with({
      :command => '/usr/local/android/android-sdk-macosx/tools/android update sdk -u -t android-15  ',
      :creates => creates,
    }) }
  end

end