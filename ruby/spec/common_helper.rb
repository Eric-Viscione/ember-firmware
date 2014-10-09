# This is configuration shared by all tests

require 'bundler/setup'

if ENV['COVERAGE']
  require 'simplecov'
  SimpleCov.start
end

Dir[File.expand_path('../support/**/*.rb', __FILE__)].each { |f| require(f) }

RSpec.configure do |config|
  config.alias_example_to(:scenario)

  config.include(FileHelper, :tmp_dir)
  config.include(FileHelperAsync, :tmp_dir_async)

  config.before(:each, :tmp_dir) do
    make_tmp_dir
  end

  config.after(:each, :tmp_dir) do
    remove_tmp_dir
  end

  config.before(:each, :tmp_dir_async) do
    make_tmp_dir_async
  end

  config.after(:each, :tmp_dir_async) do
    remove_tmp_dir_async
  end
  
  config.order = 'random'
end