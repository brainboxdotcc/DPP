project('dpp',
        'cpp',
        version : '10.0.7',
        default_options : ['warning_level=3', 'cpp_std=c++17'])

# Filesystem support
fs = import('fs')
# Pkg-Config file generation
pkgconf = import('pkgconfig')

cfg = configuration_data()
cfg.set('DPP_BUILD', 1)

# Generated with fd . src/dpp -e cpp -x echo "'{}',"
src = [
  'src/dpp/cluster_sync_calls.cpp',
  'src/dpp/emoji.cpp',
  'src/dpp/queues.cpp',
  'src/dpp/sslclient.cpp',
  'src/dpp/automod.cpp',
  'src/dpp/cluster/voice.cpp',
  'src/dpp/etf.cpp',
  'src/dpp/cluster/emoji.cpp',
  'src/dpp/cluster/automod.cpp',
  'src/dpp/guild.cpp',
  'src/dpp/cluster/gateway.cpp',
  'src/dpp/cluster/user.cpp',
  'src/dpp/cluster/timer.cpp',
  'src/dpp/cluster/channel.cpp',
  'src/dpp/cluster/dm.cpp',
  'src/dpp/cluster/appcommand.cpp',
  'src/dpp/cluster/template.cpp',
  'src/dpp/cluster/guild.cpp',
  'src/dpp/cluster/role.cpp',
  'src/dpp/cluster/json_interface.cpp',
  'src/dpp/cluster/sticker.cpp',
  'src/dpp/cluster/scheduled_event.cpp',
  'src/dpp/cluster/message.cpp',
  'src/dpp/cluster/webhook.cpp',
  'src/dpp/cluster/stage_instance.cpp',
  'src/dpp/cluster/invite.cpp',
  'src/dpp/cluster/guild_member.cpp',
  'src/dpp/voicestate.cpp',
  'src/dpp/wsclient.cpp',
  'src/dpp/discordclient.cpp',
  'src/dpp/cluster/confirmation.cpp',
  'src/dpp/dns.cpp',
  'src/dpp/discordevents.cpp',
  'src/dpp/user.cpp',
  'src/dpp/cluster/thread.cpp',
  'src/dpp/cluster.cpp',
  'src/dpp/cache.cpp',
  'src/dpp/discordvoiceclient.cpp',
  'src/dpp/channel.cpp',
  'src/dpp/events/message_delete_bulk.cpp',
  'src/dpp/events/guild_ban_add.cpp',
  'src/dpp/events/user_update.cpp',
  'src/dpp/events/logger.cpp',
  'src/dpp/events/invite_create.cpp',
  'src/dpp/events/automod_rule_delete.cpp',
  'src/dpp/events/guild_stickers_update.cpp',
  'src/dpp/events/guild_scheduled_event_delete.cpp',
  'src/dpp/events/guild_role_delete.cpp',
  'src/dpp/events/guild_integrations_update.cpp',
  'src/dpp/events/resumed.cpp',
  'src/dpp/events/integration_update.cpp',
  'src/dpp/events/thread_create.cpp',
  'src/dpp/events/thread_members_update.cpp',
  'src/dpp/events/guild_scheduled_event_user_add.cpp',
  'src/dpp/events/typing_start.cpp',
  'src/dpp/events/guild_role_update.cpp',
  'src/dpp/events/voice_state_update.cpp',
  'src/dpp/events/guild_role_create.cpp',
  'src/dpp/events/message_create.cpp',
  'src/dpp/events/guild_join_request_delete.cpp',
  'src/dpp/events/message_reaction_remove.cpp',
  'src/dpp/events/message_update.cpp',
  'src/dpp/events/thread_member_update.cpp',
  'src/dpp/events/guild_member_update.cpp',
  'src/dpp/events/stage_instance_update.cpp',
  'src/dpp/events/thread_list_sync.cpp',
  'src/dpp/events/message_reaction_remove_emoji.cpp',
  'src/dpp/events/guild_emojis_update.cpp',
  'src/dpp/events/thread_delete.cpp',
  'src/dpp/events/guild_scheduled_event_update.cpp',
  'src/dpp/events/automod_rule_create.cpp',
  'src/dpp/events/guild_delete.cpp',
  'src/dpp/events/thread_update.cpp',
  'src/dpp/events/guild_members_chunk.cpp',
  'src/dpp/events/guild_scheduled_event_create.cpp',
  'src/dpp/events/message_reaction_remove_all.cpp',
  'src/dpp/events/stage_instance_create.cpp',
  'src/dpp/events/integration_create.cpp',
  'src/dpp/events/guild_create.cpp',
  'src/dpp/events/channel_update.cpp',
  'src/dpp/events/integration_delete.cpp',
  'src/dpp/events/automod_rule_execute.cpp',
  'src/dpp/events/channel_create.cpp',
  'src/dpp/events/channel_pins_update.cpp',
  'src/dpp/events/automod_rule_update.cpp',
  'src/dpp/events/message_delete.cpp',
  'src/dpp/events/guild_ban_remove.cpp',
  'src/dpp/events/message_reaction_add.cpp',
  'src/dpp/events/interaction_create.cpp',
  'src/dpp/events/ready.cpp',
  'src/dpp/events/invite_delete.cpp',
  'src/dpp/events/guild_member_add.cpp',
  'src/dpp/events/guild_scheduled_event_user_remove.cpp',
  'src/dpp/events/channel_delete.cpp',
  'src/dpp/events/voice_server_update.cpp',
  'src/dpp/events/webhooks_update.cpp',
  'src/dpp/events/presence_update.cpp',
  'src/dpp/events/guild_member_remove.cpp',
  'src/dpp/events/guild_update.cpp',
  'src/dpp/events/stage_instance_delete.cpp',
  'src/dpp/dtemplate.cpp',
  'src/dpp/managed.cpp',
  'src/dpp/voiceregion.cpp',
  'src/dpp/commandhandler.cpp',
  'src/dpp/auditlog.cpp',
  'src/dpp/integration.cpp',
  'src/dpp/utility.cpp',
  'src/dpp/slashcommand.cpp',
  'src/dpp/role.cpp',
  'src/dpp/scheduled_event.cpp',
  'src/dpp/httpsclient.cpp',
  'src/dpp/prune.cpp',
  'src/dpp/invite.cpp',
  'src/dpp/stage_instance.cpp',
  'src/dpp/permissions.cpp',
  'src/dpp/message.cpp',
  'src/dpp/webhook.cpp',
  'src/dpp/dispatcher.cpp',
  'src/dpp/ban.cpp',
  'src/dpp/application.cpp',
  'src/dpp/snowflake.cpp',
  'src/dpp/presence.cpp',
]

cpp = meson.get_compiler('cpp')

if cpp.has_function('prctl', prefix : '#include <sys/prctl.h>')
  cfg.set('HAVE_PRCTL', 1)
endif

if cpp.has_function('pthread_setname_np', prefix : '#include <pthread.h>')
  cfg.set('HAVE_PTHREAD_SETNAME_NP', 1)
  if cpp.compiles('''
#include <thread>
int main() {
    pthread_setname_np("ThreadName");
    return 0;
}
''')
    cfg.set('HAVE_SINGLE_PARAMETER_SETNAME_NP', 1)
    message('Using single-argument variant of pthread_setname_np')
  elif cpp.compiles('''
#include <thread>
int main() {
    pthread_setname_np(pthread_self(), "ThreadName");
    return 0;
}
''')
    cfg.set('HAVE_TWO_PARAMETER_SETNAME_NP', 1)
    message('Using two-argument variant of pthread_setname_np')
  endif
endif
if build_machine.system() == 'linux'
  message('Checking for ability to update autogenerated files')
  php_exe = find_program('php', required : false)
  if php_exe.found()
    message('Checking for update to autogenerated files')
    result = run_command(php_exe,
                [
                  meson.project_source_root() / 'buildtools' / 'make_struct.php',
                  '\\Dpp\\Generator\\SyncGenerator'
                ], check : false)
    if result.returncode() == 0
      message('No change needed.')
    elif result.returncode() == 1
      message('Autoregenerated cluster_sync_calls.{cpp,h}')
    endif
  else
    warning('PHP is required to autoregenerate files.')
  endif
endif

need_cpp20 = false
if get_option('dpp_coro')
  warning('/// Enabled experimental coroutine support! ///')
  need_cpp20 = true
  if meson.get_compiler('cpp').get_id() == 'msvc'
    add_project_arguments('/await:strict', language : 'cpp')
  else
    add_project_arguments('-fcoroutines', language : 'cpp')
  endif
  cfg.set('DPP_CORO', 1)

  php_exe = find_program('php', required : false)
  if php_exe.found()
    run_command(php_exe,
                [
                  meson.project_source_root() / 'buildtools' / 'make_struct.php',
                  '\\Dpp\\Generator\\CoroGenerator',
                ], check : false)
    if result.returncode() == 0
      message('No change needed.')
    elif result.returncode() == 1
      message('Autoregenerated cluster_coro_calls.h')
    endif
  else
    warning('PHP is required to autoregenerate files.')
  endif
endif

deps = [
  dependency('nlohmann_json'),
  dependency('openssl'),
  dependency('zlib'),
  dependency('threads'),
]

if get_option('voice_support')
  voice_deps = [
    dependency('opus', required : false),
    dependency('libsodium', required : false),
  ]

  voice_support = true
  foreach dep : voice_deps
    if not dep.found()
      voice_support = false
      break
    endif
  endforeach

  if voice_support
    message('libsodium and libopus detected! VOICE support will be enabled!')
    deps += voice_deps
    cfg.set('HAVE_VOICE', 1)
  else
    warning('Could not detect libsodium or libopus. VOICE support will be disabled!')
  endif
endif

git_exe = find_program('git', required : false)

if not git_exe.found() and fs.is_dir(meson.project_source_root() / '.git')
  error('You are using a git version of D++ but don\'t have git installed. Install git (NOT \'gh\') and try again.')
elif fs.is_dir(meson.project_source_root() / '.git')
  warning('/// Building git version. Be aware that git versions may be unstable! ///')
endif

configure_file(output : 'config.h', configuration : cfg)
cfginc = include_directories('.')

overrides = []
if need_cpp20
  overrides += 'cpp_std=c++20'
endif

dpp_lib = library('dpp',
                  src,
                  dependencies : deps,
                  include_directories : [ 'include', cfginc ],
                  override_options : overrides,
                 )

dpp_dep = declare_dependency(
  link_with : dpp_lib,
  include_directories : [ 'include' ],
)
pkgconf.generate(dpp_lib,
                name : 'D++',
                filebase : 'dpp',
                description: 'An incredibly lightweight C++ Discord library.',
                url : 'https://dpp.dev/')

if get_option('dpp_build_test')
  dpp_test = executable('unittest',
                        ['src/unittest.cpp', 'src/test.cpp'],
                        include_directories : 'src',
                        dependencies : dpp_dep)
endif
