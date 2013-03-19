require 'spec_helper'

describe "android::sdk" do
  
  context 'default' do
    let(:facts) { {
      :operatingsystem => 'CentOS',
      :kernel => 'Linux',
      :osfamily => 'RedHat',
      :architecture => 'x86_64'
    } }
    
  end
  
  context '64bit RedHat' do
    let(:facts) { {
      :operatingsystem => 'CentOS',
      :kernel => 'Linux',
      :osfamily => 'RedHat',
      :architecture => 'x86_64'
    } }
    it { should contain_package('glibc.i686') }
    it { should contain_package('zlib.i686') }
    it { should contain_package('libstdc++.i686') }
  end
  
  context '64bit Debian' do
    let(:facts) { {
      :operatingsystem => 'Ubuntu',
      :kernel => 'Linux',
      :osfamily => 'Debian',
      :architecture => 'x86_64'
    } }
    it { should contain_package('ia32-libs') }
  end
  
  
  
  
end