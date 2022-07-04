/*
 * This file is pretty much an exact copy of what's in wlroots logind session backend:
 * https://github.com/swaywm/wlroots/blob/master/backend/session/logind.c
 * and in Daniel Stone kms-quads:
 * https://gitlab.freedesktop.org/daniels/kms-quads/-/blob/master/logind.c
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "sd-dbus.h"


/*
 * Invoking the GetSession systemd-logind D-Bus interface function gets
 * the D-Bus session object path string used in communications
 */
static char *find_session_path(sd_bus *bus, char *id) {
  sd_bus_message *msg = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int ret = 0;

  ret = sd_bus_call_method(bus,
                          "org.freedesktop.login1",
                          "/org/freedesktop/login1",
                          "org.freedesktop.login1.Manager",
                          "GetSession", &error, &msg, "s", id);
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_call_method: %s", error.message);
    goto exit_session_path;
  }

  const char *path = NULL;
  ret = sd_bus_message_read(msg, "o", &path);
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_message_read: %s", strerror(-ret));
    goto exit_session_path;
  }

  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);

  return strndup(path, strnlen(path, 100));

exit_session_path:
  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);

  return NULL;
}


/*
 * Invoking the Activate systemd-logind D-Bus interface function activates
 * a given session via the sd_bus object and session object path string.
 */
static int session_activate(sd_bus *bus, char *path) {
  int ret = 0;
  sd_bus_message *msg = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  ret = sd_bus_call_method(bus,
                          "org.freedesktop.login1", path,
                          "org.freedesktop.login1.Session",
                          "Activate", &error, &msg, "");
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_call_method: %s", error.message);
  }

  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);
  return ret >= 0;
}


/*
 * Invoking TakeControl systemd-logind D-Bus interface function allows
 * process to exclusively manage a device for that session.
 */
static int take_control(sd_bus *bus, char *path) {
  int ret = 0;
  sd_bus_message *msg = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  ret = sd_bus_call_method(bus,
                          "org.freedesktop.login1", path,
                          "org.freedesktop.login1.Session",
                          "TakeControl", &error, &msg, "b", false);
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_call_method: %s", error.message);
  }

  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);
  return ret >= 0;
}


/*
 * The ReleaseControl systemd-logind D-Bus interface function allows for one to release
 * control over the current active session via the sd_bus object and D-Bus object path string
 * This function also finishes up the free'ing of sd_bus related objects
 */
static void release_session_control(sd_bus *bus, char *path) {
  sd_bus_message *msg = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  if (sd_bus_call_method(bus, "org.freedesktop.login1", path,
      "org.freedesktop.login1.Session", "ReleaseControl", &error, &msg, "") < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_call_method: %s", error.message);
  }

  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);
  sd_bus_unref(bus);
}


/* Create logind session to access devices without being root */
int uvr_sd_session_create(struct uvr_sd_session *uvrsd) {
  int ret = 0;

  /* If there's a session active for the current process then just use that */
  if (sd_pid_get_session(getpid(), &(uvrsd->id)) == 0) goto start_session;

  /*
   * Find any active sessions for the user.
   * Only if the process isn't part of an active session itself
   */
  ret = sd_uid_get_display(getuid(), &(uvrsd->id));
  if (ret < 0 && ret != -ENODATA) {
    uvr_utils_log(UVR_DANGER, "[x] sd_uid_get_display: %s", strerror(-ret));
    return -1;
  }

  char *type = NULL; /* Check that the available session type is a tty */
  ret = sd_session_get_type(uvrsd->id, &type);
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_session_get_type: %s", strerror(-ret));
    uvr_utils_log(UVR_DANGER, "[x] Couldn't get a session type for session '%s'", uvrsd->id);
    return -1;
  }

  if (type[0] != 't' || type[1] != 't' || type[2] != 'y') {
    uvr_utils_log(UVR_DANGER, "[x] Unfortunately for you, the available session is not a tty :{");
    free(type); return -1;
  }

  free(type);

  /* A seat is a collection of devices */
  char *seat = NULL;
  ret = sd_session_get_seat(uvrsd->id, &seat);
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_session_get_seat: %s", strerror(-ret));
    return -1;
  }

  /* check if return seat var is the defualt seat in systemd */
  if (!strncmp(seat, "seat0", strlen("seat0"))) {
    unsigned vtn;
    /* Check if virtual terminal number exists for this session */
    if (sd_session_get_vt(uvrsd->id, &vtn) < 0) {
      uvr_utils_log(UVR_DANGER, "[x] sd_session_get_vt: %s", strerror(errno));
      uvr_utils_log(UVR_DANGER, "[x] Session not running in virtual terminal");
      free(seat); return -1;
    }
  }

  free(seat);

start_session:
  /* Connect user to system bus */
  ret = sd_bus_default_system(&(uvrsd->bus));
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_default_system: %s", strerror(-ret));
    uvr_utils_log(UVR_DANGER, "[x] Failed to open D-Bus connection");
    return -1;
  }

  /* Get D-Bus object path string */
  uvrsd->path = find_session_path(uvrsd->bus, uvrsd->id);
  if (!uvrsd->path)
    return -1;

  /* Activate the session */
  if (session_activate(uvrsd->bus, uvrsd->path) == -1)
    return -1;

  /* Take control of the session */
  if (take_control(uvrsd->bus, uvrsd->path) == -1)
    return -1;

  uvr_utils_log(UVR_SUCCESS, "Logind session successfully loaded at: session id - %s | session path - '%s'", uvrsd->id, uvrsd->path);

  return 0;
}


int uvr_sd_session_take_control_of_device(struct uvr_sd_session *uvrsd, const char *devpath) {
  sd_bus_message *msg = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int fd = -1, ret = 0;

  if (!uvrsd->path) {
    uvr_utils_log(UVR_DANGER, "[x] Must have an active logind session inorder to take control over a device");
    uvr_utils_log(UVR_DANGER, "[x] Must first make a call to uvr_sd_session_create()");
    return fd;
  }

  struct stat st;
  if (stat(devpath, &st) < 0) {
    uvr_utils_log(UVR_DANGER, "[x] stat('%s'): %s", devpath, strerror(errno));
    return fd;
  }

  ret = sd_bus_call_method(uvrsd->bus,
                          "org.freedesktop.login1", uvrsd->path,
                          "org.freedesktop.login1.Session",
                          "TakeDevice", &error, &msg, "uu",
                          major(st.st_rdev), minor(st.st_rdev));
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_call_method('%s'): %s", devpath, error.message);
    goto exit_logind_take_dev;
  }

  int paused = 0;
  ret = sd_bus_message_read(msg, "hb", &fd, &paused);
  if (ret < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_message_read('%s'): %s", devpath, strerror(-ret));
    goto exit_logind_take_dev;
  }

  /*
   * The original fd seems to be closed when the message is free'd just clone it.
   */
  fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
  if (fd == -1) {
    uvr_utils_log(UVR_DANGER, "[x] Failed to clone file descriptor for '%s': %s", devpath, strerror(errno));
    goto exit_logind_take_dev;
  }

  uvr_utils_log(UVR_SUCCESS, "Allowing %s : %d to be managed", devpath, fd);

exit_logind_take_dev:
  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);
  return fd;
}


void uvr_sd_session_release_device(struct uvr_sd_session *uvrsd, int fd) {
  sd_bus_message *msg = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  struct stat st;
  if (fstat(fd, &st) < 0) {
    uvr_utils_log(UVR_DANGER, "[x] fstat: %s", strerror(errno));
    return;
  }

  if (sd_bus_call_method(uvrsd->bus, "org.freedesktop.login1", uvrsd->path, "org.freedesktop.login1.Session",
      "ReleaseDevice", &error, &msg, "uu", major(st.st_rdev), minor(st.st_rdev)) < 0) {
    uvr_utils_log(UVR_DANGER, "[x] sd_bus_call_method('%d'): %s", fd, error.message);
  }

  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);
  close(fd);
}


void uvr_sd_session_destroy(struct uvr_sd_session *uvrsd) {
  if (uvrsd->bus && uvrsd->path)
    release_session_control(uvrsd->bus, uvrsd->path);
  free(uvrsd->id);
  free(uvrsd->path);
}
