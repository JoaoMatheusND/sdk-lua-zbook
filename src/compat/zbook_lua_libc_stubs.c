/*************************************************************************
 * @file zbook_lua_libc_stubs.c
 *
 * @brief POSIX file I/O stubs for Lua on Picolibc without Zephyr's POSIX
 * device I/O layer.
 *
 * lua_zephyr links the whole Lua core, including liolib/loslib and
 * luaL_loadfilex(), which reach fopen()/fdopen()/freopen(). With the Zephyr
 * SDK >= 1.0 (Picolibc only, no Newlib) those need open/close/read/write/
 * lseek, which Zephyr only provides with CONFIG_POSIX_DEVICE_IO and
 * CONFIG_POSIX_FILE_SYSTEM. Scripts are embedded (or loaded through
 * lua_zephyr's own zephyr.fs), so file access through libc is never needed:
 * these weak stubs fail with ENOSYS, making Lua report "cannot open" instead
 * of the link failing. Any real implementation overrides them.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#if defined(CONFIG_ZBOOK_LUA_LIBC_STUBS)

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/toolchain.h>

int __weak open(const char *path, int flags, ...)
{
	ARG_UNUSED(path);
	ARG_UNUSED(flags);
	errno = ENOSYS;

	return -1;
}

int __weak close(int fd)
{
	ARG_UNUSED(fd);
	errno = ENOSYS;

	return -1;
}

ssize_t __weak read(int fd, void *buf, size_t len)
{
	ARG_UNUSED(fd);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	errno = ENOSYS;

	return -1;
}

ssize_t __weak write(int fd, const void *buf, size_t len)
{
	ARG_UNUSED(fd);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	errno = ENOSYS;

	return -1;
}

off_t __weak lseek(int fd, off_t offset, int whence)
{
	ARG_UNUSED(fd);
	ARG_UNUSED(offset);
	ARG_UNUSED(whence);
	errno = ENOSYS;

	return -1;
}

#endif /* CONFIG_ZBOOK_LUA_LIBC_STUBS */
